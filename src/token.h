#ifndef TOKEN_H_
#define TOKEN_H_

#include <buf/buf.h>
#include <stddef.h>
#include <str/str.h>

#include "token_type.h"

typedef struct {
    TokenType type;
    str text;
    size_t position;
} Token;

static inline Token token_new(TokenType type, str text, size_t position) {
    return (Token){type, text, position};
}

typedef BUF(Token) TokenBuf;

#endif  // TOKEN_H_
