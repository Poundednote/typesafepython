#include <string>
#include <assert.h>

#include "tables.h"

// FIXME: I took this hash function of stackoverflow please
// replace with something that is more secure and robust after more research
uint32_t SymbolTable::hash(std::string &string, SymbolTableEntry *scope)
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

SymbolTableEntry *SymbolTable::lookup(std::string &string,
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

SymbolTableEntry *SymbolTable::insert(Arena *arena, std::string string,
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

        // FIXME: find out why this isn't initilising std::string correctly
        *new_entry = SymbolTableEntry();
        new_entry->key.identifier = string;
        new_entry->key.scope = scope;
        new_entry->value = *value;

//        assert(new_entry->value.node != nullptr);

        return new_entry;
}

SymbolTableEntry *SymbolTable::insert_function(Arena *arena, std::string string,
                                               SymbolTableEntry *scope,
                                               SymbolTableValue *value)
{
        SymbolTableEntry *entry = this->insert(arena, string, scope, value);

        entry->value.static_type.function.custom_symbol = entry;

        return entry;
}

Tables Tables::init(Arena *arena)
{
        Tables tables = Tables();
        tables.symbol_table =
                (SymbolTable *)arena->alloc(sizeof(SymbolTable));
        // use placement new for initilisation because calling symbol tables
        // constructor naively causes a copy
        // of the entire struct on the stack and can cause overflow
        // FIXME: find out why this isn't initilising std::string correctly
        new (tables.symbol_table) SymbolTable();

        tables.import_list =
                (ImportList *)arena->alloc(sizeof(*tables.import_list));
        new (tables.symbol_table) ImportList();

        tables.builtin_types = (TypeInfo *)arena->alloc(
                sizeof(*tables.builtin_types) * (int)TypeInfoType::SIZE);

        tables.symbol_table =
                (SymbolTable *)arena->alloc(sizeof(*tables.symbol_table));

        for (int i = 0; i < (int)TypeInfoType::SIZE; ++i) {
                new (&tables.builtin_types[i]) TypeInfo();
                tables.builtin_types[i].type = (TypeInfoType)i;
        }

        return tables;
}
