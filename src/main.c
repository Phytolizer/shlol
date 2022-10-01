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

        Parser parser = parser_new(line);
        red_prompt = parser_parse(&parser);
        str_free(line);
    }

    linenoiseHistorySave("shlol.history");
}
