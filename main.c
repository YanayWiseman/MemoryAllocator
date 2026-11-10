#include <stdio.h>

typedef struct chunk
{
    size_t size;
    
} chunk;

void* myMalloc(size_t size);
void free(void* ptr);

int main(void)
{
    printf("Hello World\n");

    void* ptr = myMalloc(32);
    return 0;
}

void* myMalloc(size_t size)
{

}

void free(void* ptr)
{

}