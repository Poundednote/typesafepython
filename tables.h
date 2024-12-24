#ifndef SYMBOLTABLE_H_
#define SYMBOLTABLE_H_

#include <string>

#include "typing.h"
#include "utils.h"

#define SYMBOL_TABLE_ARRAY_SIZE 4096

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
        SymbolTableEntry *insert_function(Arena *arena, std::string string,
                                 SymbolTableEntry *scope,
                                 SymbolTableValue *value);
        uint32_t hash(std::string &string, SymbolTableEntry *scope);
        SymbolTableEntry *lookup(std::string &string, SymbolTableEntry *scope);
};

struct ImportList {
        AstNode *list[4096];
        uint64_t list_index = 0;
};

//TODO bounds checking for builtin_type_table
struct Tables {
        SymbolTable *symbol_table;
        //TODO make these globals
        TypeInfo *builtin_types;
        ImportList *import_list;
        static Tables init(Arena *arena);
};

#endif // SYMBOLTABLE_H_
