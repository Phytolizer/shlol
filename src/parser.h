#ifndef PARSER_H_
#define PARSER_H_

#include "ast.h"
#include "lexer.h"

typedef struct {
    Lexer lexer;
} Parser;

Parser parser_new(str source);
SyntaxTree parser_parse(Parser* parser);

#endif  // PARSER_H_
