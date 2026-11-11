#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

typedef struct ChunkHeader
{
    size_t size;
    bool isFree; 
    struct ChunkHeader* next;
} ChunkHeader;

ChunkHeader* head = NULL;

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
    if(size == 0 || size >= (PTRDIFF_MAX - sizeof(ChunkHeader)))
    {
        errno = EINVAL;
        return NULL;
    }

    // Find free block of the requested size
    ChunkHeader* curr = head;
    while(curr != NULL)
    {
        if(curr->isFree && curr->size >= size)
        {
            curr->isFree = false;
            return curr + 1;
        }
        curr = curr->next;
    }

    void* ptr = mmap(NULL, sizeof(ChunkHeader) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(ptr == MAP_FAILED)
    {
        return NULL;
    }

    ChunkHeader* header = (ChunkHeader*)ptr;
    header->isFree = false;
    header->size = size;
    header->next = NULL;
    
    // Increment by sizeof(ChunkHeader)
    return header + 1;
}

void myFree(void* ptr)
{
    if(ptr == NULL)
    {
        return;
    }

    // Find pointer to the chunk's header
    ChunkHeader* casted = (ChunkHeader*)ptr;
    ChunkHeader* ptrToHeader = casted - 1;
    
    // Avoid double frees
    if(ptrToHeader->isFree)
    {
        return;
    }

    // Mark as free
    ptrToHeader->isFree = true;

    // Combine to the linked list
    ChunkHeader* curr = head;
    while(curr)
    {
        if(curr->next == NULL)
        {
            curr->next = ptrToHeader;
            break;
        }
        curr = curr->next;
    }
}