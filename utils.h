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

struct Token;
struct Arena {
        void *memory = nullptr;
        size_t capacity = 0;
        uint64_t offset = 0;

        static inline Arena init(size_t reserve);
        void *alloc(size_t size);
        void clear();
        void destroy()
        {
                VirtualFree(this->memory, 0, MEM_RELEASE);
        }
};

struct ReadFileResult {
        char *contents;
        size_t filesize;
};

char *read_entire_file();

#endif // UTILS_H_
