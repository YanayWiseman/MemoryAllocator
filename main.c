#include <stdio.h>
#include <stdlib.h>
#include "MemoryAllocator.h"

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
