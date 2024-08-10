#include <stdio.h>

#include "utils.h"

inline Arena Arena::init(size_t reserve) {
    Arena arena = {};

#ifdef _WIN32
    arena.memory = VirtualAlloc(0, reserve,
                                MEM_RESERVE, PAGE_NOACCESS);

    VirtualAlloc(arena.memory, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
#endif

    arena.capacity = PAGE_SIZE;
    arena.offset = 0;

    return arena;
}

//TODO make sure items have correct alignment
void *Arena::alloc(size_t size) {
    char *new_base = ((char *)this->memory+this->offset);
    if (this->offset+size >= this->capacity) {
        size_t bytes_to_alloc = PAGE_SIZE;
        char *alloc_base = (char *)this->memory + this->capacity;

        if (size > PAGE_SIZE) {
            // cast to avoid unsigned underflow
            bytes_to_alloc = size - ((int64_t)this->capacity - (int64_t)this->offset);
        }

        VirtualAlloc(alloc_base, bytes_to_alloc, MEM_COMMIT, PAGE_READWRITE);

        if (!alloc_base) {
            perror("Last allocation failed");
            exit(1);
        }

        this->capacity += bytes_to_alloc;
    }

    assert(this->offset < this->capacity);
    this->offset += size;
    return (void *)new_base;
};

inline SubArena SubArena::init(Arena *backing_arena, size_t size) {
    SubArena sub_arena = {};
    sub_arena.memory = backing_arena->alloc(size);
    sub_arena.capacity = size;
    sub_arena.offset = 0;

    return sub_arena;
}

inline void *SubArena::alloc(size_t size) {
    char *new_base = ((char *)this->memory+this->offset);
    if (this->offset+size >= this->capacity) {
        fprintf_s(stderr, "SubArena reached maximum capcity");
    }

    assert(this->offset < this->capacity);
    this->offset += size;
    return (void *)new_base;
}

inline void SubArena::destroy(SubArena *sub_arena, Arena *backing_arena) {
    backing_arena->offset -= sub_arena->capacity;
}

inline uint32_t SymbolTable::hash(std::string &string, SymbolTableEntry *scope) {
    uint32_t hash = 0;
    for (int i = 0; i < string.length(); ++i) {
        hash = hash * 101 + string[i];
    }

    if (scope != nullptr) {
        for (int i = 0; i < scope->key.identifier.length(); ++i) {
            hash = hash * 101 + scope->key.identifier[i];
        }
    }

    return hash % SYMBOL_TABLE_ARRAY_SIZE;
}

inline SymbolTableEntry *SymbolTable::lookup(std::string &string, SymbolTableEntry *scope) {
    SymbolTableEntry *entry = &this->table[hash(string, scope)];
    if (entry->key.identifier == "") {
        return nullptr;
    }

    if (string == "") {
        return nullptr;
    }

    while(entry) {
        if (entry->key.identifier == string &&
            entry->key.scope == scope) {

            return entry;
        }

        entry = entry->next_in_table;
    }

    return nullptr;
}

inline SymbolTableEntry *SymbolTable::insert(Arena *arena, std::string string,
                                             SymbolTableEntry *scope,
                                             SymbolTableValue value) {

    SymbolTableEntry *entry = &this->table[this->hash(string, scope)];

    if (this->lookup(string, scope) != nullptr) {
        entry->value = value;
    }

    // Collision
    else if (entry->key.identifier != "") {
        entry->next_in_table = (SymbolTableEntry *)arena->alloc(sizeof(SymbolTableEntry));
        *entry->next_in_table = SymbolTableEntry();
    }

    else {
        entry->key.identifier = string;
        entry->key.scope = scope;
        entry->value = value;
    }

    return entry;
}

inline CompilerTables CompilerTables::init(Arena *arena) {
    CompilerTables compiler_tables = {};
    compiler_tables.variable_table = (SymbolTable *)arena->alloc(
        sizeof(SymbolTable[3]));
    *compiler_tables.variable_table = SymbolTable();
    compiler_tables.function_table = compiler_tables.variable_table + 1;
    *compiler_tables.function_table = SymbolTable();
    compiler_tables.class_table = compiler_tables.variable_table+2;
    *compiler_tables.class_table = SymbolTable();
    compiler_tables.builtin_types = (TypeInfo *)arena->alloc(
        sizeof(TypeInfo[(int)TypeInfoType::SIZE]));

    for (int i = 0; i < (int)TypeInfoType::SIZE; ++i) {
        compiler_tables.builtin_types[i].type = (TypeInfoType)i;
    }

    return compiler_tables;
}
