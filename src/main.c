#include <errno.h>
#include <stddef.h>
// order dependent
#include <buf/buf.h>
#include <hedley/hedley.h>
#include <linenoise.h>
#include <println/println.h>
#include <stdbool.h>
#include <stdio.h>
#include <str/str.h>
#include <sys/wait.h>
#include <unistd.h>

#define scope(begin, end) for (bool i = (begin, false); !i; (i = true, end))
#define defer(expr) for (bool i = false; !i; (i = true, expr))

typedef BUF(char*) RawWordList;
typedef BUF(str) WordList;

typedef bool BuiltinCallback(WordList args);

typedef struct {
    str name;
    BuiltinCallback* callback;
} BuiltinWord;

static bool cd_command(WordList argv) {
    bool result = false;
    if (argv.len == 1) {
        chdir(getenv("HOME"));
    } else if (argv.len == 2) {
        str path = str_dup(argv.ptr[1]);
        chdir(path.ptr);
        str_free(path);
    } else {
        printfln("cd: too many arguments");
        result = true;
    }
    return result;
}

static const BuiltinWord BUILTIN_WORDS[] = {
    {str_lit_c("cd"), cd_command},
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

static bool execute_command(str line) {
    str_tok_state state;
    str_tok_init(&state, line, str_lit(" \t"));
    WordList words = BUF_NEW;
    str word;
    while (str_tok(&word, &state)) {
        BUF_PUSH(&words, word);
    }
    bool result = true;
    uint64_t i = 0;
    bool negate_result = false;
    if (str_eq(str_ref(words.ptr[i]), str_lit("!"))) {
        negate_result = true;
        i++;
    }
    IsBuiltinResult builtin_result = is_builtin(words.ptr[i]);
    if (builtin_result.is_builtin) {
        bool failed = builtin_result.cb(words);
        failed = negate_result ? !failed : failed;
        result = failed;
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            RawWordList argv = BUF_NEW;
            for (uint64_t j = i; j < words.len; j++) {
                str word_dup = str_dup(words.ptr[j]);
                BUF_PUSH(&argv, HEDLEY_CONST_CAST(char*, word_dup.ptr));
            }
            BUF_PUSH(&argv, NULL);
            execvp(argv.ptr[0], argv.ptr);
            printfln(str_fmt ": %s", str_arg(words.ptr[i]), strerror(errno));
            exit(1);
        }
        int status;
        waitpid(pid, &status, 0);
        bool failed = WEXITSTATUS(status) != 0;
        failed = negate_result ? !failed : failed;
        result = failed;
    }
    BUF_FREE(words);
    return result;
}

static bool execute_list(str line) {
    bool failed;
    bool skip_next = false;
    while (true) {
        line = str_trim_left(line, str_lit(" \t"));
        if (str_is_empty(line)) {
            break;
        }
        typedef enum {
            OP_AND,
            OP_OR,
            OP_NONE,
        } Op;
        size_t command_end = line.len;
        size_t command_len = 0;
        Op op = OP_NONE;
        str_find_result and_op = str_find(line, str_lit("&&"));
        if (and_op.found) {
            command_end = and_op.pos;
            op = OP_AND;
            command_len = 2;
        }
        str_find_result or_op = str_find(line, str_lit("||"));
        if (or_op.found && or_op.pos < command_end) {
            command_end = or_op.pos;
            op = OP_OR;
            command_len = 2;
        }
        str command = str_upto(line, command_end);
        if (skip_next) {
            skip_next = false;
        } else {
            failed = execute_command(command);
            if ((op == OP_AND && failed) || (op == OP_OR && !failed)) {
                skip_next = true;
            }
        }
        line = str_after(line, command_end + command_len);
    }
    return failed;
}

static bool execute_statements(str line) {
    bool failed;
    while (true) {
        line = str_trim_left(line, str_lit(" \t"));
        if (str_is_empty(line)) {
            break;
        }
        size_t command_end = line.len;
        size_t command_len = 0;
        str_find_result semicolon_op = str_find(line, str_lit(";"));
        if (semicolon_op.found) {
            command_end = semicolon_op.pos;
            command_len = 1;
        }
        str command = str_upto(line, command_end);
        failed = execute_list(command);
        line = str_after(line, command_end + command_len);
    }
    return failed;
}

int main(void) {
    linenoiseHistoryLoad("shlol.history");

    bool red_prompt = false;

    while (true) {
        char* raw_line = linenoise(red_prompt ? "\x1b[31m$ \x1b[0m" : "$ ");
        if (raw_line == NULL) {
            break;
        }

        linenoiseHistoryAdd(raw_line);
        str line = str_acquire(raw_line);

        red_prompt = execute_statements(line);
        str_free(line);
    }

    linenoiseHistorySave("shlol.history");
}
