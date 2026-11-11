#ifndef MEMORY_ALLOCATOR
#define MEMORY_ALLOCATOR

#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#define ALIGNMENT 16

typedef struct ChunkHeader
{
    size_t size;
    bool isFree;
    struct ChunkHeader* next;
} ChunkHeader __attribute__((aligned(ALIGNMENT)));

ChunkHeader* head = NULL;


void* myMalloc(size_t size);
void myFree(void* ptr);
void* myCalloc(size_t num, size_t size);
void* myRealloc(void* ptr, size_t newSize);
size_t roundUpToAlignment(size_t num);

#endif  