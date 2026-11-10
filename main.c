#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>

typedef struct chunk
{
    size_t size;
    bool isFree; 
    struct chunk* next;
} chunk;

chunk temp = {32, false, NULL};

chunk* chunkPtr = NULL;

void* myMalloc(size_t size);
void myFree(void* ptr);

int main(void)
{
    printf("Hello World\n");
    void* programBreakPtr = sbrk(0);


    void* ptr = myMalloc(32);
    if(ptr == NULL)
    {
        perror("Failed to allocate memory.\n");
        return EXIT_FAILURE;
    }
    myFree(ptr);

    return EXIT_SUCCESS;
}

void* myMalloc(size_t size)
{
    // mmap fails with EINVAL if size is 0
    if(size == 0)
    {
        return NULL;
    }
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(ptr == MAP_FAILED)
    {
        return NULL;
    }
    chunkPtr = &temp;

    return ptr;
}

void myFree(void* ptr)
{
    if(ptr != NULL || chunkPtr == NULL)
    {
        return;
    }
    if(munmap(ptr, chunkPtr->size) != 0)
    {
        perror("Failed to free allocated memory.\n");
    } 
}