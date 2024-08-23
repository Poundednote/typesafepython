#ifndef TYPING_H_
#define TYPING_H_

#include "utils.h"
#include "parser.h"

static void fail_typing_with_debug(AstNode *node);
static bool is_num_type(TypeInfo type_info);
static int type_parse_tree(AstNode *node, Arena *parse_arena, Arena *scope_stack,
                           CompilerTables *copmiler_tables,
                           SymbolTable *table_to_look);
static bool unions_are_equal(TypeInfo *union_a, TypeInfo *union_b, TypeInfo *visited_list);

#endif // TYPING_H_
