#ifndef TYPING_H_
#define TYPING_H_

#include "tpython.h"
#include "utils.h"

struct AstNode;
struct SymbolTable;
struct CompilerTables;
struct SymbolTableEntry;
struct TypeInfo;

introspect enum class TypeInfoType {
        UNKNOWN = 0,
        ANY = 1,
        INTEGER = 2,
        FLOAT = 3,
        STRING = 4,
        BOOLEAN = 5,
        NONE = 6,
        COMPLEX = 7,

        // Parameterised/NonTrivial
        LIST = 8,
        DICT = 9,
        KVPAIR = 10,
        UNION = 11,
        CLASS = 12,

        SIZE,
};

struct TypeInfoList {
        TypeInfo *item_type;
};

struct TypeInfoDict {
        TypeInfo *key_type;
        TypeInfo *val_type;
};

struct TypeInfoKVpair : public TypeInfoDict {};

struct TypeInfoUnion {
        TypeInfo *left;
        TypeInfo *right;
};

struct TypeInfo {
        TypeInfoType type = TypeInfoType::UNKNOWN;
        SymbolTableEntry *custom_symbol;

        union {
                TypeInfoList list;
                TypeInfoUnion union_type;
                TypeInfoDict dict;
                TypeInfoKVpair kvpair;
        };
};

static bool is_num_type(TypeInfo type_info);
static int type_parse_tree(AstNode *node, Arena *parse_arena,
                           SubArena *scope_stack,
                           CompilerTables *copmiler_tables,
                           SymbolTable *table_to_look);

static bool unions_are_equal(TypeInfo *union_a, TypeInfo *union_b,
                             TypeInfo *visited_list, size_t list_size);

#endif // TYPING_H_
