#include "ast.h"

#include <assert.h>
#include <stdlib.h>

Command* simple_command_new(WordList args, bool negated) {
    SimpleCommand* command = malloc(sizeof(SimpleCommand));
    assert(command != NULL);
    command->base.type = COMMAND_TYPE_SIMPLE;
    command->args = args;
    command->negated = negated;
    return (Command*)command;
}

Command* subshell_command_new(Statements* statements) {
    SubshellCommand* command = malloc(sizeof(SubshellCommand));
    assert(command != NULL);
    command->base.type = COMMAND_TYPE_SUBSHELL;
    command->statements = statements;
    return (Command*)command;
}

CommandList command_list_new(void) {
    CommandList list = {BUF_NEW, BUF_NEW};
    return list;
}

Statements* statements_new(void) {
    Statements* statements = malloc(sizeof(Statements));
    assert(statements != NULL);
    statements->lists = (CommandListBuf)BUF_NEW;
    return statements;
}

void command_free(Command* command) {
    switch (command->type) {
        case COMMAND_TYPE_SIMPLE:
            BUF_FREE(((SimpleCommand*)command)->args);
            break;
        case COMMAND_TYPE_SUBSHELL:
            statements_free(((SubshellCommand*)command)->statements);
            break;
    }
}

void command_list_free(CommandList list) {
    for (uint64_t i = 0; i < list.commands.len; i++) {
        command_free(list.commands.ptr[i]);
    }
    BUF_FREE(list.commands);
    BUF_FREE(list.ops);
}

void statements_free(Statements* statements) {
    for (uint64_t i = 0; i < statements->lists.len; i++) {
        command_list_free(statements->lists.ptr[i]);
    }
    BUF_FREE(statements->lists);
    free(statements);
}

void syntax_tree_free(SyntaxTree tree) {
    statements_free(tree.root);
}
