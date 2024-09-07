#include <string>
#include <assert.h>

#include "tables.h"

// FIXME: I took this hash function of stackoverflow please
// replace with something that is more secure and robust after more research
inline uint32_t SymbolTable::hash(std::string &string, SymbolTableEntry *scope)
{
        uint32_t hash = 0;
        for (int i = 0; i < string.length(); ++i) {
                hash = hash * 101 + string[i];
        }

        while (scope) {
                for (int i = 0; i < scope->key.identifier.length(); ++i) {
                        hash = hash * 101 + scope->key.identifier[i];
                }

                scope = scope->key.scope;
        }

        return hash % SYMBOL_TABLE_ARRAY_SIZE;
}

inline SymbolTableEntry *SymbolTable::lookup(std::string &string,
                                             SymbolTableEntry *scope)
{
        SymbolTableEntry *entry = &this->table[hash(string, scope)];
        if (entry->key.identifier == "") {
                return nullptr;
        }

        if (string == "") {
                return nullptr;
        }

        while (entry) {
                if (entry->key.identifier == string &&
                    entry->key.scope == scope) {
                        //assert(entry->value.node != nullptr);
                        return entry;
                }

                entry = entry->next_in_table;
        }

        return nullptr;
}

inline SymbolTableEntry *SymbolTable::insert(Arena *arena, std::string string,
                                             SymbolTableEntry *scope,
                                             SymbolTableValue *value)
{
        SymbolTableEntry *entry = this->lookup(string, scope);

        if (entry != nullptr) {
                entry->value = *value;
                return entry;
                //assert(entry->value.node != nullptr);
        }

        SymbolTableEntry *new_entry = &this->table[hash(string, scope)];
        if (new_entry->key.identifier != "") {
                // find an empty slot in the linked list
                while (new_entry->next_in_table) {
                        new_entry = new_entry->next_in_table;
                }

                new_entry->next_in_table = (SymbolTableEntry *)arena->alloc(
                        sizeof(*entry->next_in_table));
                *new_entry->next_in_table = SymbolTableEntry();
                new_entry->next_in_table->value = *value;

                new_entry = new_entry->next_in_table;
        }

        new_entry->key.identifier = string;
        new_entry->key.scope = scope;
        new_entry->value = *value;

        assert(new_entry->value.node != nullptr);

        return new_entry;
}

inline CompilerTables CompilerTables::init(Arena *arena)
{
        CompilerTables compiler_tables = {};
        compiler_tables.variable_table = (SymbolTable *)arena->alloc(
                sizeof(*compiler_tables.variable_table) * 3);
        // placement new to call constructors without allocating on stack
        new (compiler_tables.variable_table) SymbolTable();
        compiler_tables.function_table = compiler_tables.variable_table + 1;
        new (compiler_tables.function_table) SymbolTable();
        compiler_tables.class_table = compiler_tables.variable_table + 2;
        new (compiler_tables.class_table) SymbolTable();
        compiler_tables.builtin_types = (TypeInfo *)arena->alloc(
                sizeof((*builtin_types)) * (int)TypeInfoType::SIZE);

        for (int i = 0; i < (int)TypeInfoType::SIZE; ++i) {
                compiler_tables.builtin_types[i].type = (TypeInfoType)i;
        }

        return compiler_tables;
}
