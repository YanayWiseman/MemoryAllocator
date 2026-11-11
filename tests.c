#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "MemoryAllocator.h"


// ----------------THIS IS AI GENERATED CODE----------------

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "[FAIL] Line %d: %s (errno=%d)\n", __LINE__, msg, errno); \
        return false; \
    } \
} while (0)

// Helper to verify heap metadata integrity across the entire linked list
static bool verifyHeapIntegrity(void)
{
    ChunkHeader* curr = head;
    size_t count = 0;
    while (curr != NULL)
    {
        // Check header alignment
        if (((uintptr_t)curr % ALIGNMENT) != 0)
        {
            fprintf(stderr, "[HEAP CORRUPT] Header %p is not %d-byte aligned\n", (void*)curr, ALIGNMENT);
            return false;
        }
        // Check payload alignment
        void* payload = (void*)(curr + 1);
        if (((uintptr_t)payload % ALIGNMENT) != 0)
        {
            fprintf(stderr, "[HEAP CORRUPT] Payload %p is not %d-byte aligned\n", payload, ALIGNMENT);
            return false;
        }
        // Check size alignment and non-zero size
        if (curr->size == 0 || (curr->size % ALIGNMENT) != 0)
        {
            fprintf(stderr, "[HEAP CORRUPT] Chunk %p has invalid size %zu\n", (void*)curr, curr->size);
            return false;
        }
        // Cycle guard
        if (++count > 100000)
        {
            fprintf(stderr, "[HEAP CORRUPT] Cycle detected in free list\n");
            return false;
        }
        curr = curr->next;
    }
    return true;
}

// 1. Test roundUpToAlignment edge cases and overflow
static bool testAlignmentHelper(void)
{
    CHECK(roundUpToAlignment(0) == 0, "roundUpToAlignment(0) should be 0");
    CHECK(roundUpToAlignment(1) == 16, "roundUpToAlignment(1) should be 16");
    CHECK(roundUpToAlignment(15) == 16, "roundUpToAlignment(15) should be 16");
    CHECK(roundUpToAlignment(16) == 16, "roundUpToAlignment(16) should be 16");
    CHECK(roundUpToAlignment(17) == 32, "roundUpToAlignment(17) should be 32");
    CHECK(roundUpToAlignment(SIZE_MAX) == 0, "roundUpToAlignment(SIZE_MAX) should overflow to 0");
    CHECK(roundUpToAlignment(SIZE_MAX - 14) == 0, "roundUpToAlignment(SIZE_MAX - 14) should overflow to 0");
    return true;
}

// 2. Test myMalloc edge cases, alignment, and write/read integrity
static bool testMallocEdgeCases(void)
{
    errno = 0;
    CHECK(myMalloc(0) == NULL && errno == EINVAL, "myMalloc(0) should fail with EINVAL");

    errno = 0;
    CHECK(myMalloc(SIZE_MAX) == NULL && errno == EINVAL, "myMalloc(SIZE_MAX) should fail with EINVAL");

    errno = 0;
    CHECK(myMalloc(PTRDIFF_MAX) == NULL && errno == EINVAL, "myMalloc(PTRDIFF_MAX) should fail with EINVAL");

    // Allocate various odd sizes and ensure 16-byte alignment and writability
    size_t testSizes[] = {1, 2, 7, 15, 16, 17, 31, 64, 127, 512, 4096, 65536};
    size_t numSizes = sizeof(testSizes) / sizeof(testSizes[0]);
    void* ptrs[12];

    for (size_t i = 0; i < numSizes; i++)
    {
        ptrs[i] = myMalloc(testSizes[i]);
        CHECK(ptrs[i] != NULL, "myMalloc failed for valid size");
        CHECK(((uintptr_t)ptrs[i] % ALIGNMENT) == 0, "Returned pointer is not 16-byte aligned");
        memset(ptrs[i], (int)(i + 1), testSizes[i]);
    }

    CHECK(verifyHeapIntegrity(), "Heap corrupted after odd-size allocations");

    // Verify data wasn't overwritten by neighboring allocations
    for (size_t i = 0; i < numSizes; i++)
    {
        uint8_t* bytes = (uint8_t*)ptrs[i];
        for (size_t b = 0; b < testSizes[i]; b++)
        {
            CHECK(bytes[b] == (uint8_t)(i + 1), "Data corruption across adjacent blocks");
        }
        myFree(ptrs[i]);
    }

    CHECK(verifyHeapIntegrity(), "Heap corrupted after freeing odd-size allocations");
    return true;
}

// 3. Test myFree NULL handling, double-free protection, splitting, and coalescing
static bool testSplittingAndCoalescing(void)
{
    // Safe NULL free
    myFree(NULL);

    // Allocate one large block and free it so it sits in the free list
    size_t bigSize = 1024;
    uint8_t* big = (uint8_t*)myMalloc(bigSize);
    CHECK(big != NULL, "Failed to allocate big block");
    myFree(big);

    // Double free should be a safe no-op
    myFree(big);
    CHECK(verifyHeapIntegrity(), "Double free corrupted the heap");

    // Split the 1024-byte free block into 3 smaller adjacent chunks
    void* p1 = myMalloc(128);
    void* p2 = myMalloc(128);
    void* p3 = myMalloc(128);
    CHECK(p1 != NULL && p2 != NULL && p3 != NULL, "Failed to split big block");
    CHECK((uint8_t*)p2 > (uint8_t*)p1 && (uint8_t*)p3 > (uint8_t*)p2, "Split blocks are not contiguous");

    // Free p1 and p3 first, then p2 in the middle to trigger BOTH forward and backward coalescing
    myFree(p1);
    myFree(p3);
    myFree(p2);
    CHECK(verifyHeapIntegrity(), "Heap corrupted after forward+backward coalescing");

    // Now we should be able to re-allocate the full 1024 bytes from the coalesced block without new mmap
    void* recombined = myMalloc(1024);
    CHECK(recombined == p1, "Coalesced block was not reused from the start of p1");
    myFree(recombined);

    return true;
}

// 4. Test myCalloc zero-initialization and multiplication overflow
static bool testCalloc(void)
{
    // Dirty some memory first to ensure calloc actually zeroes reused blocks
    void* dirty = myMalloc(256);
    CHECK(dirty != NULL, "Failed to allocate dirty buffer");
    memset(dirty, 0xFF, 256);
    myFree(dirty);

    uint8_t* zeroed = (uint8_t*)myCalloc(16, 16);
    CHECK(zeroed != NULL, "myCalloc(16, 16) failed");
    for (size_t i = 0; i < 256; i++)
    {
        CHECK(zeroed[i] == 0, "myCalloc did not zero-initialize memory");
    }
    myFree(zeroed);

    // Test overflow detection
    errno = 0;
    void* overflowPtr = myCalloc(SIZE_MAX / 2 + 1, 4);
    CHECK(overflowPtr == NULL && errno == ENOMEM, "myCalloc did not catch multiplication overflow");

    CHECK(verifyHeapIntegrity(), "Heap corrupted after myCalloc tests");
    return true;
}

// 5. Test myRealloc: NULL ptr, zero size, in-place reuse, shrinking, and growing
static bool testRealloc(void)
{
    // realloc(NULL, size) == malloc(size)
    uint8_t* ptr = (uint8_t*)myRealloc(NULL, 10);
    CHECK(ptr != NULL, "myRealloc(NULL, 10) failed");
    for (int i = 0; i < 10; i++) ptr[i] = (uint8_t)(0xA0 + i);

    // Same aligned bucket (10 -> 12 both align to 16): must return the EXACT same pointer
    void* sameBucket = myRealloc(ptr, 12);
    CHECK(sameBucket == ptr, "myRealloc reallocated unnecessarily within same aligned size");

    // Grow beyond 16 bytes -> must allocate new block and preserve data
    uint8_t* grown = (uint8_t*)myRealloc(ptr, 128);
    CHECK(grown != NULL && grown != ptr, "myRealloc failed to grow block");
    for (int i = 0; i < 10; i++)
    {
        CHECK(grown[i] == (uint8_t)(0xA0 + i), "myRealloc lost data when growing");
    }
    // Fill rest of 128-byte buffer
    for (int i = 10; i < 128; i++) grown[i] = (uint8_t)(i & 0xFF);

    // Shrink 128 -> 24 bytes: must not overflow and must preserve first 24 bytes
    uint8_t* shrunk = (uint8_t*)myRealloc(grown, 24);
    CHECK(shrunk != NULL, "myRealloc failed when shrinking");
    for (int i = 0; i < 10; i++)
    {
        CHECK(shrunk[i] == (uint8_t)(0xA0 + i), "myRealloc corrupted initial bytes on shrink");
    }
    for (int i = 10; i < 24; i++)
    {
        CHECK(shrunk[i] == (uint8_t)(i & 0xFF), "myRealloc corrupted remaining bytes on shrink");
    }

    // realloc(ptr, 0) == free(ptr) and returns NULL
    void* freed = myRealloc(shrunk, 0);
    CHECK(freed == NULL, "myRealloc(ptr, 0) should return NULL");

    CHECK(verifyHeapIntegrity(), "Heap corrupted after myRealloc tests");
    return true;
}

// 6. High-volume randomized stress test ("like there's no tomorrow")
static bool testRandomStress(void)
{
    #define NUM_SLOTS 256
    #define ITERATIONS 25000

    typedef struct {
        uint8_t* ptr;
        size_t size;
        uint8_t pattern;
    } Slot;

    Slot slots[NUM_SLOTS] = {0};
    srand(42); // Deterministic seed for reproducibility

    for (size_t iter = 0; iter < ITERATIONS; iter++)
    {
        size_t idx = rand() % NUM_SLOTS;
        int op = rand() % 100;

        if (slots[idx].ptr == NULL)
        {
            // Allocate new block (70% malloc, 30% calloc)
            size_t sz = (rand() % 2048) + 1;
            uint8_t pat = (uint8_t)((rand() % 254) + 1);

            if (op < 70)
            {
                slots[idx].ptr = (uint8_t*)myMalloc(sz);
                CHECK(slots[idx].ptr != NULL, "Stress test myMalloc returned NULL");
                memset(slots[idx].ptr, pat, sz);
            }
            else
            {
                slots[idx].ptr = (uint8_t*)myCalloc(1, sz);
                CHECK(slots[idx].ptr != NULL, "Stress test myCalloc returned NULL");
                for (size_t b = 0; b < sz; b++)
                {
                    CHECK(slots[idx].ptr[b] == 0, "Stress test myCalloc non-zero byte");
                }
                memset(slots[idx].ptr, pat, sz);
            }
            slots[idx].size = sz;
            slots[idx].pattern = pat;
        }
        else if (op < 45)
        {
            // Verify pattern before freeing
            for (size_t b = 0; b < slots[idx].size; b++)
            {
                CHECK(slots[idx].ptr[b] == slots[idx].pattern, "Stress test memory corruption detected before free");
            }
            myFree(slots[idx].ptr);
            slots[idx].ptr = NULL;
            slots[idx].size = 0;
        }
        else if (op < 85)
        {
            // Realloc to a random smaller or larger size
            size_t newSz = (rand() % 4096) + 1;
            size_t minSz = (slots[idx].size < newSz) ? slots[idx].size : newSz;

            uint8_t* resized = (uint8_t*)myRealloc(slots[idx].ptr, newSz);
            CHECK(resized != NULL, "Stress test myRealloc returned NULL");
            for (size_t b = 0; b < minSz; b++)
            {
                CHECK(resized[b] == slots[idx].pattern, "Stress test data lost during myRealloc");
            }
            uint8_t newPat = (uint8_t)((rand() % 254) + 1);
            memset(resized, newPat, newSz);
            slots[idx].ptr = resized;
            slots[idx].size = newSz;
            slots[idx].pattern = newPat;
        }
        else
        {
            // Verify pattern in-place without freeing
            for (size_t b = 0; b < slots[idx].size; b++)
            {
                CHECK(slots[idx].ptr[b] == slots[idx].pattern, "Stress test silent memory corruption");
            }
        }
            // Periodically scan the entire heap linked list
        if ((iter % 1000) == 0)
        {
            CHECK(verifyHeapIntegrity(), "Heap integrity failed mid-stress test");
        }
    }

    // Clean up all remaining live allocations
    for (size_t i = 0; i < NUM_SLOTS; i++)
    {
        if (slots[i].ptr != NULL)
        {
            for (size_t b = 0; b < slots[i].size; b++)
            {
                CHECK(slots[i].ptr[b] == slots[i].pattern, "Final cleanup memory corruption");
            }
            myFree(slots[i].ptr);
        }
    }
    CHECK(verifyHeapIntegrity(), "Heap integrity failed after stress test cleanup");
    
    return true;
}

// Master test runner returning true if all checks pass
bool runAllAllocatorTests(void)
{
    if (!testAlignmentHelper())       return false;
    if (!testMallocEdgeCases())       return false;
    if (!testSplittingAndCoalescing()) return false;
    if (!testCalloc())                return false;
    if (!testRealloc())               return false;
    if (!testRandomStress())          return false;
    return true;
}

int main(void)
{
    bool allPassed = runAllAllocatorTests();
    printf("All allocator tests passed: %s\n", allPassed ? "true" : "false");
    return allPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}