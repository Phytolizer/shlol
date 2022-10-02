#include "parser.h"

#include <println/println.h>

static Statements* parse_statements(Parser* parser, TokenBuf* tokens, Statements* existing);
static bool token_is_list_op(Token token);
static void parse_list(Parser* parser, TokenBuf* tokens, CommandList* out_list);
static Command* parse_command(Parser* parser, TokenBuf* tokens);

Parser parser_new(str source) {
    return (Parser){
        .lexer = lexer_new(source),
        .needs_more_input = false,
    };
}

ParseResult parser_parse(Parser* parser) {
    TokenBuf tokens = lex(&parser->lexer);

    TokenBuf temp_tokens = BUF_AS_REF(tokens);
    Statements* result = parse_statements(parser, &temp_tokens, NULL);
    if (parser->errored) {
        BUF_FREE(tokens);
        return (ParseResult)SUM_NOTHING;
    }
    if (parser->needs_more_input) {
        PartialParse partial = {.tree = {.root = result}};
        BUF_FREE(tokens);
        return (ParseResult)SUM_JUST(SUM_RIGHT(partial));
    }
    if (temp_tokens.len > 0 && temp_tokens.ptr[0].type != TOKEN_TYPE_EOF) {
        fprintfln(
            stderr,
            "col %zu: syntax error (token '" str_fmt "')",
            temp_tokens.ptr[0].position,
            str_arg(temp_tokens.ptr[0].text)
        );
        statements_free(result);
        result = NULL;
    }
    BUF_FREE(tokens);
    SyntaxTree tree = {.root = result};
    return (ParseResult)SUM_JUST(SUM_LEFT(tree));
}

ParseResult parser_resume_parse(Parser* parser, PartialParse partial, str source) {
    parser->needs_more_input = false;
    parser->lexer = lexer_new(source);
    TokenBuf tokens = lex(&parser->lexer);

    TokenBuf temp_tokens = BUF_AS_REF(tokens);
    Statements* result = parse_statements(parser, &temp_tokens, partial.tree.root);
    if (parser->errored) {
        BUF_FREE(tokens);
        syntax_tree_free(partial.tree);
        return (ParseResult)SUM_NOTHING;
    }
    if (parser->needs_more_input) {
        PartialParse partial = {.tree = {.root = result}};
        BUF_FREE(tokens);
        return (ParseResult)SUM_JUST(SUM_RIGHT(partial));
    }

    if (temp_tokens.len > 0 && temp_tokens.ptr[0].type != TOKEN_TYPE_EOF) {
        fprintfln(
            stderr,
            "col %zu: syntax error (token '" str_fmt "')",
            temp_tokens.ptr[0].position,
            str_arg(temp_tokens.ptr[0].text)
        );
        parser->errored = true;
        statements_free(result);
        result = NULL;
    }
    BUF_FREE(tokens);
    SyntaxTree tree = {.root = result};
    return (ParseResult)SUM_JUST(SUM_LEFT(tree));
}

static Statements* parse_statements(Parser* parser, TokenBuf* tokens, Statements* existing) {
    Statements* result = existing;
    if (result == NULL) {
        result = statements_new();
    }
    CommandList partial_list = existing != NULL ? BUF_LAST(existing->lists) : command_list_new();
    parse_list(parser, tokens, &partial_list);
    CommandList list = partial_list;  // no longer "partial" -- hopefully
    if (existing != NULL) {
        BUF_LAST(existing->lists) = list;
    } else {
        BUF_PUSH(&result->lists, list);
    }
    while (tokens->len > 0 && tokens->ptr[0].type == TOKEN_TYPE_SEMI) {
        BUF_SHIFT(tokens, 1);
        CommandList list = command_list_new();
        parse_list(parser, tokens, &list);
        BUF_PUSH(&result->lists, list);
    }

    if (tokens->len == 0) {
        fprintfln(stderr, "internal error: skipped EOF");
        abort();
    }
    return result;
}

static bool token_is_list_op(Token token) {
    return token.type == TOKEN_TYPE_AMP_AMP || token.type == TOKEN_TYPE_PIPE_PIPE;
}

static void parse_list(Parser* parser, TokenBuf* tokens, CommandList* out_list) {
    if (tokens->len == 0) {
        // empty list is allowed
        return;
    }

    Command* command = parse_command(parser, tokens);
    BUF_PUSH(&out_list->commands, command);
    while (tokens->len > 0 && token_is_list_op(tokens->ptr[0])) {
        Op op = tokens->ptr[0].type == TOKEN_TYPE_AMP_AMP ? OP_AND : OP_OR;
        BUF_PUSH(&out_list->ops, op);
        BUF_SHIFT(tokens, 1);
        if (tokens->len == 0 || tokens->ptr[0].type == TOKEN_TYPE_EOF) {
            parser->needs_more_input = true;
            break;
        }
        command = parse_command(parser, tokens);
        BUF_PUSH(&out_list->commands, command);
    }
}

static bool token_is_nonword(Token lhs, void* user) {
    (void)user;
    return lhs.type != TOKEN_TYPE_WORD;
}

static Command* parse_command(Parser* parser, TokenBuf* tokens) {
    uint64_t first_nonword;
    BUF_INDEX(*tokens, NULL, token_is_nonword, &first_nonword);
    TokenBuf argv = BUF_BEFORE(*tokens, first_nonword);
    BUF_SHIFT(tokens, first_nonword);

    if (argv.len == 0) {
        if (tokens->ptr[0].type == TOKEN_TYPE_LPAREN) {
            // subshell
            BUF_SHIFT(tokens, 1);
            Statements* statements = parse_statements(parser, tokens, NULL);

            if (tokens->len == 0 || tokens->ptr[0].type != TOKEN_TYPE_RPAREN) {
                fprintfln(
                    stderr,
                    "col %zu: syntax error (expected ')' after subshell command, not '" str_fmt
                    "')",
                    tokens->ptr[0].position,
                    str_arg(tokens->ptr[0].text)
                );
                parser->errored = true;
                statements_free(statements);
                return NULL;
            }
            BUF_SHIFT(tokens, 1);
            return subshell_command_new(statements);
        }
        fprintfln(
            stderr,
            "col %zu: syntax error (expected command, not '" str_fmt "')",
            tokens->ptr[0].position,
            str_arg(tokens->ptr[0].text)
        );
        parser->errored = true;
        return NULL;
    }
    bool negated = false;
    while (argv.len > 0 && argv.ptr[0].type == TOKEN_TYPE_WORD &&
           str_eq(argv.ptr[0].text, str_lit("!"))) {
        negated = !negated;
        BUF_SHIFT(&argv, 1);
    }
    if (argv.len == 0) {
        fprintfln(
            stderr,
            "col %zu: syntax error (expected command, not '" str_fmt "')",
            tokens->ptr[0].position,
            str_arg(tokens->ptr[0].text)
        );
        parser->errored = true;
        return NULL;
    }
    WordList args = BUF_NEW;
    for (uint64_t i = 0; i < argv.len; i++) {
        BUF_PUSH(&args, argv.ptr[i].text);
    }
    return simple_command_new(args, negated);
}
