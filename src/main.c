#include <stddef.h>
// order dependent
#include <buf/buf.h>
#include <errno.h>
#include <hedley/hedley.h>
#include <linenoise.h>
#include <println/println.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdnoreturn.h>
#include <str/str.h>
#include <sys/wait.h>
#include <unistd.h>

#include "executor.h"
#include "parser.h"

#define scope(begin, end) for (bool i = (begin, false); !i; (i = true, end))
#define defer(expr) for (bool i = false; !i; (i = true, expr))

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

        while (str_has_suffix(line, str_lit("\\"))) {
            char* raw_line = linenoise("> ");
            if (raw_line == NULL) {
                break;
            }

            linenoiseHistoryAdd(raw_line);
            str line2 = str_acquire(raw_line);
            str full_line = str_cat_ret(str_upto(line, str_len(line) - 1), line2);
            str_free(line);
            str_free(line2);
            line = full_line;
        }

        Parser parser = parser_new(str_ref(line));
        SyntaxTree tree = parser_parse(&parser);
        defer((syntax_tree_free(tree), str_free(line))) {
            int status = execute_tree(tree);
            red_prompt = status != 0;
        }
    }

    linenoiseHistorySave("shlol.history");
}
