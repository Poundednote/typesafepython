#ifndef TYPING_H_
#define TYPING_H_

#include "main.h"
#include "utils.h"

struct AstNode;
struct SymbolTable;
struct Tables;
struct SymbolTableEntry;
struct TypeInfo;

introspect enum class TypeInfoType {
        ANY = 0,
        INTEGER = 1,
        FLOAT = 2,
        STRING = 3,
        BOOLEAN = 4,
        COMPLEX = 5,
        NONE = 6,
        NOT_IMPLEMENTED = 7,

        // Parameterised/NonTrivial
        LIST = 8,
        DICT = 9,
        KVPAIR = 10,
        UNION = 11,
        CLASS = 12,
        FUNCTION = 13,

        UNKNOWN,
        SIZE,
};

introspect struct TypeInfoList {
        TypeInfo *item_type;
};

introspect struct TypeInfoParameterised {
        TypeInfo *parameters;
};

introspect struct TypeInfoDict {
        TypeInfo *key_type;
        TypeInfo *val_type;
};

introspect struct TypeInfoKVpair {
        TypeInfo *val_type;
        TypeInfo *key_type;
};

introspect struct TypeInfoUnion {
        TypeInfo *left;
        TypeInfo *right;
};

introspect struct TypeInfoClass {
        // This is required for getting the classes scope information
        // from variable names
        SymbolTableEntry *custom_symbol;
};

introspect struct TypeInfoFunction {
        TypeInfo *return_type;
        SymbolTableEntry *custom_symbol;
};

struct TypeInfo {
        TypeInfoType type;

        union {
                TypeInfoList list;
                TypeInfoUnion union_type;
                TypeInfoDict dict;
                TypeInfoKVpair kvpair;
                TypeInfoClass class_type;
                TypeInfoFunction function;
        };

        TypeInfo *next;
};

static bool is_num_type(TypeInfo type_info);
static int type_parse_tree(AstNode *node, Arena *parse_arena,
                           Arena *scope_stack, Tables *tables,
                           const char *filename);
static bool unions_are_equal(TypeInfo union_a, TypeInfo union_b,
                             TypeInfo *visited_list, size_t list_size);
static bool static_types_is_rhs_equal_lhs(TypeInfo lhs, TypeInfo rhs);

#endif // TYPING_H_
