#ifndef SYMBOLTABLE_H_
#define SYMBOLTABLE_H_

#include <string>

#include "typing.h"
#include "utils.h"

#define SYMBOL_TABLE_ARRAY_SIZE (4096)

struct SymbolTableEntry;
struct AstNode;

struct SymbolTableKey {
        std::string identifier;
        SymbolTableEntry *scope;
};

struct SymbolTableValue {
        TypeInfo static_type;
        AstNode *node;
};

struct SymbolTableEntry {
        SymbolTableKey key;
        SymbolTableValue value;

        SymbolTableEntry *next_in_table;
};

struct SymbolTable {
        SymbolTableEntry table[SYMBOL_TABLE_ARRAY_SIZE];

        SymbolTableEntry *insert(Arena *arena, std::string string,
                                 SymbolTableEntry *scope,
                                 SymbolTableValue *value);
        uint32_t hash(std::string &string, SymbolTableEntry *scope);
        SymbolTableEntry *lookup(std::string &string, SymbolTableEntry *scope);
};

//TODO bounds checking for builtin_type_table
struct CompilerTables {
        SymbolTable *variable_table;
        SymbolTable *function_table;
        SymbolTable *class_table;
        TypeInfo *builtin_types;

        static CompilerTables init(Arena *arena);
};

#endif // SYMBOLTABLE_H_
