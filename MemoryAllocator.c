#include "MemoryAllocator.h"

ChunkHeader* head = NULL;

/*
    Function allocates a 16 byte aligned block of memory from the free list
    (splitting large chunks when possible) or requests new anonymous pages from the OS using mmap()
    Input: size is the number of bytes to allocate
    Output: a 16 byte aligned pointer to the allocated memory, or NULL on failure
*/

void* myMalloc(size_t size)
{
    // Align to the nearest multiple of 16
    size = roundUpToAlignment(size);
    // mmap() fails with EINVAL for these cases
    if(size == 0 || size >= (PTRDIFF_MAX - sizeof(ChunkHeader)))
    {
        errno = EINVAL;
        return NULL;
    }

    // Traverse the chunk list using a First Fit search strategy
    ChunkHeader* curr = head;
    while(curr)
    {
        // Check if the remaining space is large enough to hold a new
        // ChunkHeader plus at least one minimum aligned payload (16 bytes)
        if(curr->isFree && curr->size >= size)  // Found a free block
        {
            if(curr->size - size >= sizeof(ChunkHeader) + ALIGNMENT)  // perform block splitting
            {
                size_t remainder = curr->size - size - sizeof(ChunkHeader);
                curr->size = size;

                // Advance by byte offsets to place the split header immediately after the newly sized payload region
                uint8_t* casted = (uint8_t*)curr;

                ChunkHeader* splitted = (ChunkHeader*)(casted + sizeof(ChunkHeader) + size);
                splitted->isFree = true;
                splitted->size = remainder;

                // Insert splitted into the linked list
                splitted->next = curr->next;
                curr->next = splitted;
            }
            // Mark block as occupied and return pointer past the ChunkHeader
            curr->isFree = false;
            return curr + 1;
        }
        curr = curr->next;
    }

    // No suitable free chunk found mean request a anonymous mapping from the OS
    void* ptr = mmap(NULL, sizeof(ChunkHeader) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(ptr == MAP_FAILED)
    {
        return NULL;
    }

    // Initialize the new header at the start of the mapped pages and prepend to list
    ChunkHeader* header = (ChunkHeader*)ptr;
    header->isFree = false;
    header->size = size;
    header->next = head;
    head = header;

    // Increment by sizeof(ChunkHeader)
    return header + 1;
}

/*
    Function frees a previously allocated block of memory, guards against double frees,
    and coalesces adjacent free chunks both forward and backward
    Input: ptr is a pointer to the allocated memory block to free
    Output: No return values
*/

void myFree(void* ptr)
{
    if(!ptr)
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
    // Forword coalescing
    // Merge with the next list node only if it is free and physically adjacent in virtual memory (since separate mmap() regions are non contiguous)
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

    // Backwords coalescing
    // Locate the immediate predecessor in the linked list and merge if it is free and physically adjacent immediately before ptrToHeader
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

/*
    Function allocates memory for an array of elements, checks for multiplication overflow, and initializes all bytes to zero
    Input: num is a number of elements to allocate and size is the byte size of each element
    Output: A 16 byte aligned pointer to the zero initialized memory, or NULL on failure
*/

void* myCalloc(size_t num, size_t size)
{
    // Prevent unsigned integer wrap-around when computing total byte count
    if (size != 0 && num > SIZE_MAX / size)
    {
        errno = ENOMEM;
        return NULL;
    }
    void* ptr = myMalloc(num * size);
    if(!ptr)
    {
        return NULL;
    }

    // Initialize everything to 0
    memset(ptr, 0, num * size);

    return ptr;
}

/*
    Function resizes a previously allocated block of memory in place if it fits, or allocates a new block, copies existing data, and frees the old block
    Input: ptr is a pointer to the existing memory block (or NULL)and newSize is the new requested size in bytes
    Output: A 16 byte aligned pointer to the resized memory block, or NULL if freed or on failure
*/

void* myRealloc(void* ptr, size_t newSize)
{
    // If ptr is NULL, behave just like malloc()
    if(!ptr)
    {
        return myMalloc(newSize);
    }
    
    // If the new size is 0, free the allocated memory
    if(!newSize)
    {
        myFree(ptr);
        return NULL;
    }

    // Check the existing block's current capacity
    ChunkHeader* ptrToHeader = ((ChunkHeader*)ptr) - 1;  // Find the Header
    size_t alignedNewSize = roundUpToAlignment(newSize);
    if (!alignedNewSize || alignedNewSize >= (PTRDIFF_MAX - sizeof(ChunkHeader)))
    {
        errno = EINVAL;
        return NULL;
    }

    // Return immediately if the current block already fits the requested size
    if (ptrToHeader->size >= alignedNewSize)
    {
        return ptr;
    }

    // Allocate a larger chunk
    void* newPtr = myMalloc(newSize);
    if(!newPtr)
    {
        return NULL;
    }
    // Copy only the minimum of old capacity and requested size to prevent
    // reading past the old buffer or overflowing the new buffer
    size_t bytesToCopy = (ptrToHeader->size < newSize) ? ptrToHeader->size : newSize;
    memcpy(newPtr, ptr, bytesToCopy);
    myFree(ptr);

    return newPtr;
}

/*
    Function rounds an unsigned number up to the nearest multiple of ALIGNMENT (16) while checking for integer overflow
    Input: num is the raw byte size to align
    Output: the aligned size rounded up to a multiple of 16, or 0 if num is 0 or overflows
*/

size_t roundUpToAlignment(size_t num)
{
    // Check overflow
    if (num > SIZE_MAX - (ALIGNMENT - 1))
    {
        return 0;
    }
    return (num + ALIGNMENT - 1) & ~(ALIGNMENT -1);
}