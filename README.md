# Custom Memory Allocator

A malloc/free, calloc and reallooc replacement in C, built on top of mmap(). I wrote it to understand how heap allocators work.

## Features

- myMalloc, myFree, myCalloc and myRealloc
- 16-byte aligned allocations
- First fit search over a linked list of chunks
- Block splitting when a free chunk is bigger than needed
- Forward and backward coalescing of adjacent free chunks
- Double free protection
- Overflow checks in myMalloc, myCalloc and the alignment helper

## Files

| File | Description |
|------|-------------|
| MemoryAllocator.c / .h | The allocator itself |
| main.c | Small demo: reads numbers, sorts them using memory from myMalloc |
| tests.c | Test suite (edge cases, splitting/coalescing, realloc, random stress test) |

## Build

Needs GCC and a one of the followig systems: Linux, WSL, macOS
since it uses mmap()

Demo program:


gcc -Wall -Wextra -g main.c MemoryAllocator.c -o memoryAllocator


Tests:


gcc -Wall -Wextra -g tests.c MemoryAllocator.c -o tests


main.c and tests.c both have their own main(), so build them separately.

## Run

Tests:


./tests


If everything works you'll see All allocator tests passed: true. On failure it prints the line and message of the failed check and exits with a non zero code.

## How it works

Every allocation gets a ChunkHeader placed right before the memory returned to the user.
It stores the chunk size, whether it's free, and a pointer to the next chunk.

- **Malloc:** the size is rounded up to a multiple of 16, then the list is searched for the first free chunk that fits. If the leftover is big enough for another header plus 16 bytes, the chunk is split. If nothing fits, a new region is requested from the OS with mmap().
- **Free:** marks the chunk as free and merges it with its neighbors if they are free and physically next to each other in memory.
- **Realloc:** if the current chunk is already big enough it's returned as is, otherwise a new chunk is allocated, the data is copied, and the old one is freed.
