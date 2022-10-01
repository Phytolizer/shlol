#include "lexer.h"

#include <ctype.h>

static char peek(const Lexer* lexer, size_t n) {
    if (lexer->position + n >= str_len(lexer->source)) {
        return '\0';
    }
    return lexer->source.ptr[lexer->position + n];
}

static char current(const Lexer* lexer) {
    return peek(lexer, 0);
}

static bool is_special(char c) {
    return isspace(c) || c == ';' || c == '(' || c == ')' || c == '&' || c == '|';
}

Lexer lexer_new(str source) {
    return (Lexer){.source = source, .position = 0};
}

TokenBuf lex(Lexer* lexer) {
    TokenBuf tokens = BUF_NEW;

    while (true) {
        while (isspace(current(lexer))) {
            lexer->position++;
        }
        size_t token_start = lexer->position;
        TokenType type = TOKEN_TYPE_BAD;
        switch (current(lexer)) {
            case '\0':
                type = TOKEN_TYPE_EOF;
                break;
            case ';':
                type = TOKEN_TYPE_SEMI;
                lexer->position++;
                break;
            case '(':
                type = TOKEN_TYPE_LPAREN;
                lexer->position++;
                break;
            case ')':
                type = TOKEN_TYPE_RPAREN;
                lexer->position++;
                break;
            case '&':
                if (peek(lexer, 1) == '&') {
                    type = TOKEN_TYPE_AMP_AMP;
                    lexer->position += 2;
                }
                break;
            case '|':
                if (peek(lexer, 1) == '|') {
                    type = TOKEN_TYPE_PIPE_PIPE;
                    lexer->position += 2;
                }
                break;
            default:
                while (!is_special(current(lexer)) && current(lexer) != '\0') {
                    lexer->position++;
                }
                type = TOKEN_TYPE_WORD;
                break;
        }

        if (type == TOKEN_TYPE_BAD) {
            break;
        }

        Token token = {
            .type = type,
            .position = token_start,
            .text = str_substr_bounds(lexer->source, token_start, lexer->position),
        };
        BUF_PUSH(&tokens, token);
        if (token.type == TOKEN_TYPE_EOF) {
            break;
        }
    }

    return tokens;
}
