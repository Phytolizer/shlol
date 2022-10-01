#include "parser.h"

#include <errno.h>
#include <println/println.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <str/strtox.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum { PURE, IMPURE } Purity;

typedef BUF(char*) RawWordList;

typedef bool BuiltinCallback(TokenBuf args);

typedef struct {
    str name;
    BuiltinCallback* callback;
} BuiltinWord;

static noreturn void exec_process(TokenBuf argv) {
    RawWordList raw_argv = BUF_NEW;
    for (uint64_t j = 0; j < argv.len; j++) {
        str word_dup = str_dup(argv.ptr[j].text);
        BUF_PUSH(&raw_argv, HEDLEY_CONST_CAST(char*, word_dup.ptr));
    }
    BUF_PUSH(&raw_argv, NULL);
    execvp(raw_argv.ptr[0], raw_argv.ptr);
    printfln(str_fmt ": %s", str_arg(argv.ptr[0].text), strerror(errno));
    exit(1);
}

static int run_process(TokenBuf argv, bool should_fork) {
    if (should_fork) {
        pid_t pid = fork();
        if (pid == 0) {
            exec_process(argv);
        }
        int status;
        waitpid(pid, &status, 0);
        return status;
    }

    exec_process(argv);
}

static bool cd_command(TokenBuf argv) {
    bool result = false;
    if (argv.len == 1) {
        chdir(getenv("HOME"));
    } else if (argv.len == 2) {
        str path = str_dup(argv.ptr[1].text);
        chdir(path.ptr);
        str_free(path);
    } else {
        printfln("cd: too many arguments");
        result = true;
    }
    return result;
}

static bool exit_command(TokenBuf argv) {
    if (argv.len == 1) {
        exit(0);
    }
    if (argv.len == 2) {
        Str2I64Result status = str2i64(argv.ptr[1].text, 10);
        if (status.err || status.endptr != str_end(argv.ptr[1].text)) {
            printfln("exit: invalid argument");
            return true;
        }
        status.value = status.value % 256;
        exit((int)status.value);
    }

    printfln("exit: too many arguments");
    return true;
}

static bool exec_command(TokenBuf argv) {
    TokenBuf actual_argv = BUF_SHIFTED(argv, 1);
    run_process(actual_argv, false);
    // unreachable
    abort();
}

static const BuiltinWord BUILTIN_WORDS[] = {
    {str_lit_c("cd"), cd_command},
    {str_lit_c("exit"), exit_command},
    {str_lit_c("exec"), exec_command},
};

typedef struct {
    bool is_builtin;
    BuiltinCallback* cb;
} IsBuiltinResult;

static IsBuiltinResult is_builtin(str word) {
    BUF(const BuiltinWord) builtin_words = BUF_ARRAY(BUILTIN_WORDS);
    for (uint64_t i = 0; i < builtin_words.len; i++) {
        if (str_eq(builtin_words.ptr[i].name, word)) {
            return (IsBuiltinResult){true, builtin_words.ptr[i].callback};
        }
    }

    return (IsBuiltinResult){false, NULL};
}

static bool parse_statements(TokenBuf* tokens, Purity purity);
static bool token_is_list_op(Token token);
static bool parse_list(TokenBuf* tokens, Purity purity);
static bool parse_command(TokenBuf* tokens, Purity purity);

Parser parser_new(str source) {
    return (Parser){.lexer = lexer_new(source)};
}

bool parser_parse(Parser* parser) {
    TokenBuf tokens = lex(&parser->lexer);

    TokenBuf temp_tokens = BUF_AS_REF(tokens);
    bool result = parse_statements(&temp_tokens, IMPURE);
    if (temp_tokens.len > 0 && temp_tokens.ptr[0].type != TOKEN_TYPE_EOF) {
        fprintfln(
            stderr,
            "col %zu: syntax error (token '" str_fmt "')",
            temp_tokens.ptr[0].position,
            str_arg(temp_tokens.ptr[0].text)
        );
        result = true;
    }
    BUF_FREE(tokens);
    return result;
}

static bool parse_statements(TokenBuf* tokens, Purity purity) {
    bool result = parse_list(tokens, purity);
    while (tokens->len > 0 && tokens->ptr[0].type == TOKEN_TYPE_SEMI) {
        BUF_SHIFT(tokens, 1);
        result = parse_list(tokens, purity);
    }

    if (purity == PURE) {
        return false;
    }

    if (tokens->len == 0) {
        fprintfln(stderr, "internal error: skipped EOF");
        abort();
    }
    return result;
}

static bool token_is_list_op(Token token) {
    return token.type == TOKEN_TYPE_AMP_AMP || token.type == TOKEN_TYPE_PIPE_PIPE;
}

static bool parse_list(TokenBuf* tokens, Purity purity) {
    if (tokens->len == 0) {
        // empty list is allowed
        return false;
    }

    bool result = parse_command(tokens, purity);
    Purity short_circuit = purity;
    while (tokens->len > 0 && token_is_list_op(tokens->ptr[0])) {
        bool is_and = tokens->ptr[0].type == TOKEN_TYPE_AMP_AMP;
        bool trigger_short_circuit = is_and ? result : !result;
        if (short_circuit == IMPURE) {
            short_circuit = trigger_short_circuit ? PURE : IMPURE;
        }
        BUF_SHIFT(tokens, 1);
        bool temp_result = parse_command(tokens, short_circuit);
        if (short_circuit == IMPURE) {
            result = temp_result;
        }
    }

    return result;
}

static bool token_is_nonword(Token lhs, void* user) {
    (void)user;
    return lhs.type != TOKEN_TYPE_WORD;
}

static bool parse_command(TokenBuf* tokens, Purity purity) {
    uint64_t first_nonword;
    BUF_INDEX(*tokens, NULL, token_is_nonword, &first_nonword);
    TokenBuf argv = BUF_BEFORE(*tokens, first_nonword);
    BUF_SHIFT(tokens, first_nonword);

    if (purity == PURE) {
        return false;
    }

    if (argv.len == 0 && tokens->ptr[0].type == TOKEN_TYPE_LPAREN) {
        // subshell
        pid_t pid = fork();
        if (pid == 0) {
            BUF_SHIFT(tokens, 1);
            exit(parse_statements(tokens, purity) ? 1 : 0);
        }

        BUF_SHIFT(tokens, 1);
        parse_statements(tokens, PURE);

        int status;
        waitpid(pid, &status, 0);

        if (tokens->len == 0 || tokens->ptr[0].type != TOKEN_TYPE_RPAREN) {
            fprintfln(
                stderr,
                "col %zu: syntax error (expected ')' after subshell command, not '" str_fmt "')",
                tokens->ptr[0].position,
                str_arg(tokens->ptr[0].text)
            );
            return true;
        }
        BUF_SHIFT(tokens, 1);
        return WEXITSTATUS(status) != 0;
    }

    bool do_negate_result = false;
    while (argv.len > 0 && str_eq(argv.ptr[0].text, str_lit("!"))) {
        BUF_SHIFT(&argv, 1);
        do_negate_result = !do_negate_result;
    }

    if (argv.len == 0 || argv.ptr[0].type == TOKEN_TYPE_EOF) {
        // empty command is allowed
        return false;
    }

    IsBuiltinResult is_builtin_result = is_builtin(argv.ptr[0].text);
    if (is_builtin_result.is_builtin) {
        bool result = is_builtin_result.cb(argv);
        return do_negate_result ? !result : result;
    }

    int status = run_process(argv, true);
    bool failed = WEXITSTATUS(status) != 0;
    return do_negate_result ? !failed : failed;
}
