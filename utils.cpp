#include <stdio.h>

#include "utils.h"

inline Arena Arena::init(size_t reserve)
{
        Arena arena = {};

#ifdef _WIN32
        arena.memory = VirtualAlloc(0, reserve, MEM_RESERVE, PAGE_NOACCESS);

        VirtualAlloc(arena.memory, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
#endif

        arena.capacity = PAGE_SIZE;
        arena.offset = 0;

        return arena;
}

//TODO make sure items have correct alignment
void *Arena::alloc(size_t size)
{
        char *new_base = ((char *)this->memory + this->offset);
        if (this->offset + size >= this->capacity) {
                size_t bytes_to_alloc = PAGE_SIZE;
                char *alloc_base = (char *)this->memory + this->capacity;

                if (size > PAGE_SIZE) {
                        // cast to avoid unsigned underflow
                        bytes_to_alloc = size - ((int64_t)this->capacity -
                                                 (int64_t)this->offset);
                }

                VirtualAlloc(alloc_base, bytes_to_alloc, MEM_COMMIT,
                             PAGE_READWRITE);

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

inline SubArena SubArena::init(Arena *backing_arena, size_t size)
{
        SubArena sub_arena = {};
        sub_arena.backing_arena = backing_arena;
        sub_arena.memory = backing_arena->alloc(size);
        sub_arena.capacity = size;
        sub_arena.offset = 0;

        return sub_arena;
}

inline void *SubArena::alloc(size_t size)
{
        char *new_base = ((char *)this->memory + this->offset);
        if (this->offset + size >= this->capacity) {
                fprintf_s(stderr, "SubArena reached maximum capcity");
        }

        assert(this->offset < this->capacity);
        this->offset += size;
        return (void *)new_base;
}

inline void SubArena::destroy()
{
        this->backing_arena->offset -= this->capacity;
}
