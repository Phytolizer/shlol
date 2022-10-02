#include "executor.h"

#include <assert.h>
#include <errno.h>
#include <println/println.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <str/strtox.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef BUF(char*) RawWordList;

typedef int BuiltinCallback(WordList argv);

typedef struct {
    str name;
    BuiltinCallback* callback;
} BuiltinWord;

static noreturn void exec_process(WordList argv) {
    RawWordList raw_argv = BUF_NEW;
    for (uint64_t j = 0; j < argv.len; j++) {
        str word_dup = str_dup(argv.ptr[j]);
        BUF_PUSH(&raw_argv, HEDLEY_CONST_CAST(char*, word_dup.ptr));
    }
    BUF_PUSH(&raw_argv, NULL);
    execvp(raw_argv.ptr[0], raw_argv.ptr);
    printfln(str_fmt ": %s", str_arg(argv.ptr[0]), strerror(errno));
    exit(1);
}

static int run_process(WordList argv, bool should_fork) {
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

static int cd_command(WordList argv) {
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

static int exit_command(WordList argv) {
    if (argv.len == 1) {
        exit(0);
    }
    if (argv.len == 2) {
        Str2I64Result status = str2i64(argv.ptr[1], 10);
        if (status.err || status.endptr != str_end(argv.ptr[1])) {
            printfln("exit: invalid argument");
            return true;
        }
        status.value = status.value % 256;
        exit((int)status.value);
    }

    printfln("exit: too many arguments");
    return true;
}

static int exec_command(WordList argv) {
    WordList actual_argv = BUF_SHIFTED(argv, 1);
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

static int execute_statements(Statements* statements);
static int execute_list(CommandList list);
static int execute_command(Command* command);
static int execute_simple_command(SimpleCommand* command);
static int execute_subshell_command(SubshellCommand* command);

int execute_tree(SyntaxTree tree) {
    return execute_statements(tree.root);
}

static int execute_statements(Statements* statements) {
    int result = 0;
    for (uint64_t i = 0; i < statements->lists.len; i++) {
        result = execute_list(statements->lists.ptr[i]);
    }
    return result;
}

static int execute_list(CommandList list) {
    assert(list.commands.len == 0 || list.commands.len - 1 == list.ops.len);
    int result = 0;
    for (uint64_t i = 0; i < list.commands.len; i++) {
        result = execute_command(list.commands.ptr[i]);
        if (list.ops.len > i) {
            Op op = list.ops.ptr[i];
            bool short_circuit;
            switch (op) {
                case OP_AND:
                    short_circuit = result != 0;
                    break;
                case OP_OR:
                    short_circuit = result == 0;
                    break;
                default:
                    abort();
            }
            if (short_circuit) {
                break;
            }
        }
    }

    return result;
}

static int execute_command(Command* command) {
    switch (command->type) {
        case COMMAND_TYPE_SIMPLE:
            return execute_simple_command((SimpleCommand*)command);
        case COMMAND_TYPE_SUBSHELL:
            return execute_subshell_command((SubshellCommand*)command);
        default:
            abort();
    }
}

static int execute_simple_command(SimpleCommand* command) {
    IsBuiltinResult is_builtin_result = is_builtin(command->args.ptr[0]);
    if (is_builtin_result.is_builtin) {
        int result = is_builtin_result.cb(command->args);
        return command->negated ? !result : result;
    }

    int status = WEXITSTATUS(run_process(command->args, true));
    return command->negated ? !status : status;
}

static int execute_subshell_command(SubshellCommand* command) {
    pid_t pid = fork();
    if (pid == 0) {
        int result = execute_statements(command->statements);
        exit(result);
    }

    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}
