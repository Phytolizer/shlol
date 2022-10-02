#ifndef AST_H_
#define AST_H_

#include <buf/buf.h>
#include <str/str.h>

typedef BUF(str) WordList;

typedef enum {
#define X(x) COMMAND_TYPE_##x,
#include "command_type.inc"
#undef X
} CommandType;

typedef struct {
    CommandType type;
} Command;

typedef struct {
    Command base;
    WordList args;
    bool negated;
} SimpleCommand;

struct Statements;

typedef struct {
    Command base;
    struct Statements* statements;
} SubshellCommand;

typedef BUF(Command*) CommandBuf;

typedef enum {
    OP_AND,
    OP_OR,
} Op;

typedef BUF(Op) OpBuf;

typedef struct {
    CommandBuf commands;
    OpBuf ops;
} CommandList;

typedef BUF(CommandList) CommandListBuf;

typedef struct Statements {
    CommandListBuf lists;
} Statements;

typedef struct {
    Statements* root;
} SyntaxTree;

Command* simple_command_new(WordList args, bool negated);
Command* subshell_command_new(Statements* statements);
CommandList command_list_new(void);
Statements* statements_new(void);
void command_free(Command* command);
void command_list_free(CommandList list);
void statements_free(Statements* statements);
void syntax_tree_free(SyntaxTree tree);

#endif  // AST_H_
