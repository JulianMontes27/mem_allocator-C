#include "main.h"

/*
 * Recursive function to find a free memory block of at least `allocation` words.
 * `hdr` is the current header we're inspecting.
 * `n` is the offset in words from the *beginning* of the memory space.
 */
header *findBlock_(header *hdr, word allocation, word n)
{
    // This checks if the current position (n) plus the requested allocation size would exceed the heap limits:
    // This ensures we don't try to allocate beyond our memory space limit
    // The -2 accounts for potential header overhead at the end
    if ((n + allocation) > (Maxwords - 2))
    {
        // Set error code to ErrNoMem (1) and return NULL
        // This uses the reterr macro defined in main.h
        reterr(ErrNoMem);
    }

    // If current block is free and large enough to satisfy our allocation request
    // This implements a "first-fit" memory allocation strategy
    if (!hdr->alloced && hdr->w >= allocation)
    {
        // We found a suitable block, return its address
        return hdr;
    }

    // Otherwise, we need to move to the next block in memory
    // Calculate the address of the next header by adding the current block's size (in bytes)
    // hdr->w contains the size in words, so multiply by 4 to get bytes
    header *next = (header *)((char *)hdr + hdr->w * 4); // Move 'w' words ahead

    // Update our position tracking (n_ = new position in words from start of memspace)
    word n_ = n + hdr->w;

    // Recursively continue the search with the next block
    // This will eventually either find a block or hit the out-of-bounds check
    return findBlock_(next, allocation, n_);
}

/*
 * Allocates a block of memory in the heap with `words` 4-byte units.
 * `hdr` is the header of the block to mark as allocated.
 * Returns a pointer to the usable memory (after the header).
 */
void *mkalloc(word words, header *hdr)
{
    // Calculate how many words into the memspace this header is located
    // This helps us verify we're not exceeding the heap size
    word wordsin = ((char *)hdr - (char *)memspace) / 4;

    // Double-check that this allocation won't exceed available memory
    // This adds an extra safety check beyond the one in findBlock_
    if (words > (Maxwords - wordsin))
    {
        reterr(ErrNoMem);
    }

    // Update the header to reflect the allocation
    // Set the size of the block (in # of words)
    hdr->w = words;
    // Mark the block as allocated (no longer free)
    hdr->alloced = true;

    // Return pointer after header (the actual usable memory)
    // This skips over the header so the user gets a pointer to the data area
    return (void *)(hdr + 1);
}

/*
 * Top-level malloc-like function. Tries to allocate a block of `bytes` size.
 * Returns pointer to allocated memory or NULL on failure.
 */
void *alloc(int32 bytes)
{
    word words;
    void *mem = (void *)memspace; // Start of our heap (defined in heap.asm)
    header *hdr = (header *)mem;

    // Round up to nearest word (4 bytes). Adding 3 (which is divisor-1) to the dividend before performing integer division is a common programming technique. The formula (n + (d-1)) / d always rounds up for positive integers. This is equivalent to the mathematical ceiling function : ⌈n / d⌉
    // This ensures proper alignment of all allocations
    words = (bytes + 3) / 4;

    // If heap is uninitialized (first allocation ever)
    // This is indicated by a zeroed-out header at the start of memspace
    if (hdr->w == 0)
    {
        // For first allocation, check if request exceeds total heap size
        if (words > Maxwords)
        {
            reterr(ErrNoMem);
        }
        // Initialize the first block and return it
        return mkalloc(words, hdr);
    }

    // For subsequent allocations, find a suitable free block in the heap
    // The findBlock macro expands to start the search from the beginning of memspace
    header *found = findBlock(hdr, words, 0);
    if (!found)
    {
        // If no suitable block was found, return NULL
        // errno will already be set by findBlock_
        return NULL;
    }

    // Mark the found block as allocated and return it
    return mkalloc(words, found);
}

/*
 * Entry point: test the allocator
 * Demonstrates basic usage of the allocation system
 */
int main(int unused argc, char **unused argv)
{
    // Attempt to allocate 2000 bytes (500 words since each word is 4 bytes)
    void *ptr;
    ptr = alloc(2000);

    // Check if allocation failed
    if (!ptr)
    {
        // Print the error code from errno
        printf("Error %d\n", errno);
        return -1;
    }


    // Memory Address:  0x565d900c       0x565d9010                           
    //              +--------------+----------------------------------+
    //              | Header (4B)  | User Data (2000B requested)      |
    //              +--------------+----------------------------------+
    //              ^              ^
    //              |              |
    //              |              +-- allocated pointer (returned to user)
    //              +-- memspace base address & header address

    // Print diagnostic information to demonstrate the memory layout
    // Shows the base address of our memory space
    printf("memspace base address: %p\n", (void *)memspace);
    // Shows the address returned to the user (after the header)
    printf("allocated pointer:     %p\n", ptr);
    // Shows where the header is located (just before the returned pointer)
    printf("header is at:          %p\n", (void *)((header *)ptr - 1));

    return 0;
}