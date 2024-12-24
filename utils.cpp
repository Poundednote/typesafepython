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

void Arena::clear()
{
        this->offset = 0;
}

ReadFileResult read_entire_file(const char *filename)
{
        ReadFileResult result = {};
        FILE *target_f;
        fopen_s(&target_f, filename, "rb");
        if (!target_f) {
                return result;
        }

        // compute filesize
        fseek(target_f, 0, SEEK_END);
        result.filesize = ftell(target_f);
        fseek(target_f, 0, SEEK_SET);
        result.contents = new char[result.filesize + 1];
        if (result.contents == nullptr) {
                // this shouldn't have happend
                exit(1);
        }

        // read contents into file
        fread_s(result.contents, result.filesize, 1, result.filesize,
                target_f);
        result.contents[result.filesize] = 0; // for safe
        fclose(target_f);
        return result;
}
