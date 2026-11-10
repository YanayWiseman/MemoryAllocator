#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>

typedef struct
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
    if(munmap(ptr, chunkPtr->size) != 0)
    {
        perror("Failed to free allocated memory.\n");
    } 
}