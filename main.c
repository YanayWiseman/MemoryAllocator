#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
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
} ChunkHeader;

ChunkHeader* head = NULL;

void* myMalloc(size_t size);
void myFree(void* ptr);
size_t roundUpToAlignment(size_t num);

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
    while(curr)
    {
        if(curr->isFree && curr->size >= size)  // Found a free block
        {
            if(curr->size - size >= sizeof(ChunkHeader) + 1)  // perform block splitting
            {
                size_t remainder = curr->size - size - sizeof(ChunkHeader);
                curr->size = size;

                uint8_t* casted = (uint8_t*)curr;

                ChunkHeader* splitted = (ChunkHeader*)(casted + sizeof(ChunkHeader) + size);
                splitted->isFree = true;
                splitted->size = remainder;

                // Insert splitted into the linked list
                splitted->next = curr->next;
                curr->next = splitted;
            }
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
    header->next = head;
    head = header;

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
    uint8_t* byteSizedPtr = (uint8_t*)casted;


    // Coalesce blocks
    // Check forwords
    if(ptrToHeader->next)  // First check if it exists
    {
        ChunkHeader* temp = ptrToHeader->next;
        if(ptrToHeader->next->isFree && byteSizedPtr + ptrToHeader->size == (uint8_t*)temp)
        {
            // Coalesce
            ChunkHeader* toCoalesce = ptrToHeader->next;
            ptrToHeader->next = ptrToHeader->next->next;  // Isolate the block in the middle
            ptrToHeader->size += toCoalesce->size + sizeof(ChunkHeader);
        }
    }

    // Check backwords
    if(head)
    {
        ChunkHeader* curr = head;
        
        // Find the node before ptrToHeader and coalesce if its free
        while(curr)
        {
            uint8_t* currBytePtr = (uint8_t*)curr;
            if(curr->next == ptrToHeader && curr->isFree && (currBytePtr + sizeof(ChunkHeader) + curr->size == (uint8_t*)ptrToHeader))
            {
                // Coalesce
                curr->next = ptrToHeader->next;
                curr->size += ptrToHeader->size + sizeof(ChunkHeader);
                break;
            }
            curr = curr->next;
        }  
    }
}

size_t roundUpToAlignment(size_t num)
{
    return (num + ALIGNMENT - 1) & ~(ALIGNMENT -1);
}