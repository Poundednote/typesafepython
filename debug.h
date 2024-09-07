#ifndef DEBUG_H_
#define DEBUG_H_
#include <stdint.h>
#include <string>

#include "meta.cpp"
#include "parser.h"
#include "typing.h"
#include "tables.h"

static void debug_print_indent(uint32_t indent);
static void debug_print_node(AstNode *node, uint32_t indent);
static void debug_print_parse_tree(AstNode *node, uint32_t indent);
const char *debug_enum_to_string(EnumMemberDefinition *enumMembers, int value_of_enum);


#endif // DEBUG_H_
