#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
#define PAGE_SIZE (0x1000)
#endif
#define KILOBYTES(n) (n * (size_t)1024)
#define MEGABYTES(n) ((KILOBYTES(n)) * 1024)
#define GIGABYTES(n) ((MEGABYTES(n)) * 1024)

#if DEBUG
#define assert(condition) if(!(condition)) {*(volatile int *)(0) = 0;}
#endif


struct Arena {
    void *memory = nullptr;
    size_t capacity = 0;
    uint32_t offset = 0;

    static inline Arena init(size_t reserve);
    void *alloc(size_t size);
    void destroy() {VirtualFree(this->memory, 0, MEM_RELEASE);}
};

struct SubArena: public Arena {
    Arena *backing_arena;

    static inline SubArena init(Arena *backing_arena, size_t capacity);
    inline void *alloc(size_t capacity);
    inline void destroy();
};

enum class TypeInfoType {
    UNKNOWN = 0,
    INTEGER = 1,
    FLOAT = 2,
    STRING = 3,
    BOOLEAN = 4,
    COMPLEX = 5,
    LIST = 6,
    DICT = 7,
    NONE = 8,

    UNION = 9,
    CUSTOM = 10,
    SIZE,
};

struct SymbolTableEntry;
struct TypeInfo;

struct TypeInfoList {
    TypeInfo *item_type;
};

struct TypeInfoUnion {
    TypeInfo *left;
    TypeInfo *right;
};

struct TypeInfo {
    TypeInfoType type;
    SymbolTableEntry *custom_symbol;

    union {
        TypeInfoList list;
        TypeInfoUnion union_type;
    };

};

#endif // UTILS_H_
