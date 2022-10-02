#ifndef PARSER_H_
#define PARSER_H_

#include <sum/sum.h>

#include "ast.h"
#include "lexer.h"

typedef struct {
    Lexer lexer;
    bool needs_more_input;
    bool errored;
} Parser;

typedef struct {
    SyntaxTree tree;
} PartialParse;

typedef SUM_EITHER_TYPE(SyntaxTree, PartialParse) Parse;
typedef SUM_MAYBE_TYPE(Parse) ParseResult;

Parser parser_new(str source);
ParseResult parser_parse(Parser* parser);
ParseResult parser_resume_parse(Parser* parser, PartialParse partial, str source);

#endif  // PARSER_H_
