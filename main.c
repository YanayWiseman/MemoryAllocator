#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>

typedef struct
{
    size_t size;
    bool isFree; 
    chunk* next;
} chunk;

void* myMalloc(size_t size);
void free(void* ptr);

int main(void)
{
    printf("Hello World\n");
    void* programBreakPtr = sbrk(0);


    void* ptr = myMalloc(32);
    return 0;
}

void* myMalloc(size_t size)
{

}

void free(void* ptr)
{

}