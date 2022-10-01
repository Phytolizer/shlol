#ifndef TOKEN_TYPE_H_
#define TOKEN_TYPE_H_

typedef enum {
#define X(x) TOKEN_TYPE_##x,
#include "token_type.inc"
#undef X
} TokenType;

#endif  // TOKEN_TYPE_H_
