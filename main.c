#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

typedef struct chunkHeader
{
    size_t size;
    bool isFree; 
    struct chunkHeader* next;
} chunkHeader;

void* myMalloc(size_t size);
void myFree(void* ptr);

int main(void)
{
    printf("Hello World\n");

    void* ptr = myMalloc(32);
    if(ptr == NULL)
    {
        perror("Failed to allocate memory.");
        return EXIT_FAILURE;
    }
    myFree(ptr);

    return EXIT_SUCCESS;
}

void* myMalloc(size_t size)
{
    // mmap fails with EINVAL for these cases
    if(size == 0 || size >= PTRDIFF_MAX)
    {
        errno = EINVAL;
        return NULL;
    }

    void* ptr = mmap(NULL, sizeof(chunkHeader) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(ptr == MAP_FAILED)
    {
        return NULL;
    }

    ptr = (chunkHeader*)ptr;
    
    return ptr + sizeof(chunkHeader);
}

void myFree(void* ptr)
{
    if(ptr == NULL || ptr - sizeof(chunkHeader) == NULL)
    {
        return;
    }

    chunkHeader* ptrToHeader = ptr - sizeof(chunkHeader);

    if(munmap(ptr, ptrToHeader->size) != 0)
    {
        perror("Failed to free allocated memory.");
    }
    ptrToHeader->isFree = true;
}