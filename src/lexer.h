#ifndef LEXER_H_
#define LEXER_H_

#include <str/str.h>

#include "token.h"

typedef struct {
    str source;
    size_t position;
} Lexer;

Lexer lexer_new(str source);

TokenBuf lex(Lexer* lexer);

#endif  // LEXER_H_
