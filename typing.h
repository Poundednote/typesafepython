#ifndef TYPING_H_
#define TYPING_H_

#include "parser.h"

static void fail_typing_with_debug(AstNode *node);
static bool is_num_type(TypeInfo type_info);
static int type_parse_tree(AstNode *node, Arena *scope_stack, CompilerTables *copmiler_tables);


#endif // TYPING_H_
