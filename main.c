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
    void* ptr = myMalloc(32);
    if(ptr == NULL)
    {
        perror("Failed to allocate memory.");
        return EXIT_FAILURE;
    }
    printf("Memory Allocated succussfully!\n");

    myFree(ptr);
    printf("Memory Freed succussfully\n");
    
    return EXIT_SUCCESS;
}

void* myMalloc(size_t size)
{
    // mmap() fails with EINVAL for these cases
    if(size == 0 || size >= (PTRDIFF_MAX - sizeof(chunkHeader)))
    {
        errno = EINVAL;
        return NULL;
    }

    void* ptr = mmap(NULL, sizeof(chunkHeader) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(ptr == MAP_FAILED)
    {
        return NULL;
    }

    chunkHeader* header = (chunkHeader*)ptr;
    header->isFree = false;
    header->size = size;
    header->next = NULL;
    
    // Increment by sizeof(chunkHeader)
    return header + 1;
}

void myFree(void* ptr)
{
    if(ptr == NULL)
    {
        return;
    }

    // Find pointer to the chunk's header
    chunkHeader* casted = (chunkHeader*)ptr;
    chunkHeader* ptrToHeader = casted - 1;

    if(munmap(ptrToHeader, ptrToHeader->size + sizeof(chunkHeader)) != 0)
    {
        perror("Failed to free allocated memory.");
    }
}