/*
 * Memory allocator with block splitting and proper initialization
 * This file contains the core logic for a simple first-fit memory allocator.
 * It manages a fixed-size heap defined in heap.asm.
*/

#include "main.h"

/*
 * Frees a block of memory previously allocated by alloc().
 * Takes a pointer `ptr` to the data part of the allocated block.
 * Marks the corresponding block header as free.
 * Includes basic validation checks.
 * Does NOT yet include coalescing.
 */
void freealloc(void *ptr)
{
    if (ptr == NULL)
    {
        // Standard C behavior: calling free(NULL) has no effect.
        printf("free: Called with NULL pointer. Doing nothing.\n");
        return;
    }

    printf("free: Attempting to free pointer %p\n", ptr);

    // Calculate the address of the block header.
    // The header is always located immediately before the data pointer returned by alloc.
    // By casting the void pointer to a header pointer and subtracting 1,
    // pointer arithmetic moves back by sizeof(header) bytes.
    header *hdr = (header *)ptr - 1;

    // --- Basic Validation Checks ---
    // These checks help prevent crashes or heap corruption from invalid free calls.
    // Check if the calculated header address is within the bounds of our memspace.
    // A valid header must be at or after the start of memspace and sufficiently before the end to potentially describe a block (at least 1 word for the header itself).

    // Calculate the offset of the header form the sstart of the memspace
    word hdr_offfset = ((char *)hdr - (char *)memspace) / 4;
    if (hdr < (header *)memspace || hdr_offfset >= Maxwords)
    {
        printf("free: Error: Invalid pointer %p (calculated header %p, offset %d) - outside heap bounds.\n", ptr, hdr, hdr_offfset);
        // In a real system, this might abort or log a critical error.
        // We'll print and return for now.
        return;
    }

    // Check if the block is currently marked as allocated.
    // Freeing a block that is already free (double-free) is an error.
    if (!hdr->alloced)
    {
        printf("free: Error: Pointer %p (header %p) is already free.\n", ptr, hdr);
        // Again, might abort or log in a real system.
        return;
    }

    // Check for a zero-sized block being freed. While unlikely with current alloc,
    // it's a safety check against corruption. A valid header should have w >= 1.
    if (hdr->w == 0)
    {
        printf("free: Error: Pointer %p (header %p) has a zero size header. Heap possibly corrupted.\n", ptr, hdr);
        return;
    }

    // --- Mark the block as free ---
    hdr->alloced = false;
    printf("free: Marked block at %p (header %p, size %d words) as free.\n", ptr, hdr, hdr->w);

    // --- Coalescing (Merging with the next block) ---
    // When a block is freed, we should check the block immediately following it. If the next block is also free, we can merge the newly freed block and the next free block into a single, larger free block. This reduces fragmentation.

    // This is located immediately after the current block being freed.
    header *next_blocks_header = (header *)((char *)hdr + hdr->w * 4);

    // Calculate the offset of the potential next header.
    word next_hdr_offset = hdr_offfset + hdr->w;

    if (next_hdr_offset < Maxwords)
    {
        if (!next_blocks_header->alloced)
        {
            // Check if the next block has a valid non-zero size (safety).
            if (next_blocks_header->w > 0)
            {
                printf("free: Coalescing with next free block at %p (size %d words).\n", next_blocks_header, next_blocks_header->w);

                hdr->w += next_blocks_header->w;
                printf("free: Merged block size is now %d words.\n", hdr->w);
            }
            else
            {
                printf("free: Warning: Next block at %p has zero size. Skipping coalescing with it.\n", next_blocks_header);
            }
        }
        else
        {
            printf("free: Next block at %p is allocated. Cannot coalesce forward.\n", next_blocks_header);
        }
    }
    else
    {
        printf("free: No next block within bounds (offset %d >= %d). Cannot coalesce forward.\n", next_hdr_offset, Maxwords);
    }

    printf("free: Free operation completed for pointer %p.\n", ptr);
};

/*
 * Recursive function to find a free memory block of at least `words_to_alloc` words.
 * It searches the heap starting from `hdr`.
 * `hdr` is the current header of the block being inspected.
 * `n` is the offset in words from the *beginning* of the memory space (`memspace`).
 * Returns a pointer to the header of a suitable free block, or NULL if none is found.
*/
header *findBlock_(header *hdr, word words_to_alloc, word n)
{
    // Debug print to help diagnose the issue
    printf("findBlock_: Looking at block at %p, size %d words, alloced=%d, n=%d, request=%d words\n", hdr, hdr->w, hdr->alloced, n, words_to_alloc);

    // Check if we have reached or gone past the end of the valid memory space.
    // If the current offset 'n' is greater than or equal to the total words available, there's no more space to search.
    if ((n >= Maxwords - 2))
    {
        printf("findBlock_: Reached end of memory space (offset %d >= %d). No suitable block found.\n", n, Maxwords);
        // Return NULL to indicate no block was found starting from this point.
        return NULL;
    }

    // We need space for the requested data (`words_to_alloc`) PLUS the header (1 word).
    word total_required_size = words_to_alloc + 1;

    // If current block is free AND large enough to satisfy our allocation request (data + header)
    if (!hdr->alloced && hdr->w >= total_required_size)
    {
        printf("findBlock_: Found suitable block at %p, size %d words (requires %d words total)\n", hdr, hdr->w, total_required_size);
        return hdr; // Found a suitable free block, return its header pointer
    }

    // If the current block is not suitable (either allocated or too small), move to the next block.

    // Calculate the address of the next header.
    // We move forward from the current header's address by the size of the current block (hdr->w), which is in words. Multiply by 4 because each word is 4 bytes.
    header *next = (header *)((char *)hdr + hdr->w * 4);

    // Update our position tracking (offset in words from the beginning).
    // The next block starts after the current block, so add the current block's size to the current offset.
    word n_ = n + hdr->w;

    printf("findBlock_: Moving to next block at %p, new offset %d words\n", next, n_);

    // Recursively continue the search with the next block's header and the updated offset.
    return findBlock_(next, words_to_alloc, n_);
}

/*
 * Allocates a block of memory in the heap with `words_to_alloc` 4-byte units.
 * `hdr` is the header of the *found* free block that is large enough.
 * Returns a pointer to the usable memory (after the header).
 * Implements block splitting for efficient memory usage when the found block is larger than needed.
*/

void *mkalloc(word words_to_alloc, header *hdr)
{
    // Calculate how many words into the memspace this header is located.
    // This is mainly for debugging output to show the block's position.
    word wordsin = ((char *)hdr - (char *)memspace) / 4;

    // Calculate the total size needed for the allocated block (data + header).
    word total_required_size = words_to_alloc + 1;

    printf("mkalloc: Attempting to allocate %d words (data + header) at offset %d words (%p)\n", total_required_size, wordsin, hdr);

    // Get the original total size of the found free block.
    word original_size = hdr->w; // Size includes its own header

    // If the original block is larger than what's required by at least 2 words, we can split it.
    // We need at least 1 word for the remainder's header + 1 word for minimum data or padding to make a valid new free block.
    if ((original_size - total_required_size) >= 2)
    {
        printf("mkalloc: Splitting block - original size %d words, allocating %d words, remainder size %d words.\n", original_size, total_required_size, original_size - total_required_size);

        // Calculate the address for the header of the new free block (the remainder). It starts immediately after the allocated portion
        header *next_hdr = (header *)((char *)hdr + total_required_size * 4);

        // Set the size of the new free block.
        next_hdr->w = original_size - total_required_size;
        next_hdr->alloced = false; // Mark the remainder block as free.
        printf("mkalloc: Created new free block (remainder) at %p, size %d words.\n", next_hdr, next_hdr->w);

        // Update the size of the block being allocated to reflect only the allocated portion.
        hdr->w = total_required_size;
    }
    // If the remainder is too small to split, we just allocate the entire block found by findBlock_.
    // In this case, hdr->w remains original_size, and no new free block is created after it.
    // This path is taken if (original_size - total_required_size) < 2.
    // Mark the block as allocated.
    hdr->alloced = true;

    printf("mkalloc: Marked block at %p as allocated, size %d words (data portion %d words).\n", hdr, hdr->w, words_to_alloc);
    // Return a pointer to the usable memory area, which is immediately *after* the header.
    // Pointer arithmetic 'hdr + 1' automatically moves the pointer by the size of 'header'.
    return (void *)(hdr + 1);
}

/*
 * Top-level malloc-like function. Tries to allocate a block of `bytes` size.
 * Rounds the requested size up to the nearest word size.
 * Handles the initial heap setup on the first allocation.
 * Finds a suitable free block using findBlock and marks it as allocated using mkalloc.
 * Returns pointer to allocated memory (the data area) or NULL on failure.
*/
void *alloc(int32 bytes)
{
    word words;
    header *hdr = (header *)memspace; // Pointer to the very first header in the heap.

    // Calculate the number of words needed for the data. Round up to the nearest word (4 bytes).
    // The expression `(bytes + 3) / 4` performs integer division equivalent to ceil(bytes / 4.0).
    words = (bytes + 3) / 4;

    printf("alloc: Request for %d bytes (%d words)\n", bytes, words);

    // Check if the heap is uninitialized. We use the size field of the first header as an indicator.
    // If hdr->w is 0, it implies the memory block is in its zero-initialized state from .bss.
    if (hdr->w == 0)
    {
        printf("alloc: First allocation detected - initializing heap metadata.\n");
        // For the very first allocation, we need enough space for the requested data words
        // PLUS the header word (1). Check if this exceeds the total heap capacity.
        if (words + 1 > Maxwords)
        {
            printf("alloc: Initial allocation request (%d words data + 1 header = %d total) exceeds total heap size (%d words).\n", words, words + 1, Maxwords);

            reterr(ErrNoMem); // Set errno to indicate out of memory and return NULL.
        }

        // Initialize the first block's header to represent the entire heap as one large free block.
        // The size of this initial block is the total heap size in words.
        hdr->w = Maxwords;
        hdr->alloced = false; // The entire heap is initially free.

        printf("alloc: Initialized heap first block with size %d words.\n", hdr->w);
    }

    // Find a suitable free block in the heap.
    // We start the search from the beginning of the heap (`memspace`).
    // The `findBlock` macro calls `findBlock_` with the correct starting address and offset 0.
    // We pass the number of words requested for *data* (`words`).
    // FIX: Corrected the parameters passed to the findBlock macro call.
    header *found = findBlock(words, 0);

    // If findBlock returns NULL, it means no suitable free block large enough was found in the heap.

    if (!found)
    {
        printf("alloc: No suitable free block found for %d words (including header).\n", words + 1);
        // findBlock_ doesn't set errno for not finding a block, so we set it here.
        errno = ErrNoMem;
        return NULL;
    }

    // Mark the found block as allocated and handle potential splitting if it's larger than needed.
    // mkalloc takes the requested *data* size in words and the header of the found block.
    return mkalloc(words, found);
}

/*
 * Simple function to print the current memory layout
 */
void print_memory_layout()
{
    header *current = (header *)memspace;
    word offset = 0;

    printf("\n==== MEMORY LAYOUT ====\n");

    while (offset < Maxwords)
    {
        printf("Block at %p (offset %d words): size=%d words, %s\n",
               current, offset, current->w,
               current->alloced ? "ALLOCATED" : "FREE");

        if (current->w == 0)
        {
            printf("  Warning: Zero-sized block detected! Layout may be corrupted.\n");
            break;
        }

        offset += current->w;
        current = (header *)((char *)current + current->w * 4);

        if (offset >= Maxwords)
        {
            break;
        }
    }

    printf("=======================\n\n");
}

/*
 * Entry point: test the allocator
 * Demonstrates basic usage of the allocation system
 */
int main(int unused argc, char **unused argv)
{
    printf("Starting memory allocator test\n");
    printf("Heap size: %d words (%d bytes)\n", Maxwords, Maxwords * 4);

    // Print initial memory layout
    print_memory_layout();

    // --- First Allocation ---
    // Attempt to allocate 8 bytes. This will be the first allocation, triggering heap initialization and splitting the large initial block.
    int8 *ptr;
    printf("\nMaking first allocation: 8 bytes\n");
    ptr = alloc(8);

    // Check if allocation failed
    if (!ptr)
    {
        printf("First allocation failed with error %d\n", errno);
        return -1;
    }

    // Print memory layout after first allocation
    print_memory_layout();

    // Make a second allocation of 16 bytes
    int8 *ptr2;
    printf("\nMaking second allocation: 16 bytes\n");
    ptr2 = alloc(16);

    // Check if second allocation failed
    if (!ptr2)
    {
        printf("Second allocation failed with error %d\n", errno);

        // Still show the memory layout
        print_memory_layout();

        // Continue execution to see the first allocation details
    }
    else
    {
        // Print memory layout after second allocation
        print_memory_layout();
    }

    // --- Freeing blocks ---
    printf("\n--- Freeing blocks ---\n");
    printf("Freeing first allocated block (8 bytes).\n");

    freealloc(ptr);

    // Print layout after freeing the first block (should show it as free).
    // Note: Forward coalescing won't happen here unless ptr2 was NULL.
    print_memory_layout();

    if (ptr2)
    {
        printf("Freeing second allocated block (16 bytes).\n");
        freealloc(ptr2); // Free the second block
    }

    // Print layout after freeing the second block.
    // Forward coalescing SHOULD happen here, merging the block freed by ptr2
    // with the large free block that followed it.
    print_memory_layout();

    // --- Diagnostic Information ---
    printf("\nMemory allocation details (pointers might be invalid if freed):\n");
    printf("memspace base address:%p\n", (void *)memspace); // Address of the start of the heap.

    return 0; // Indicate successful program execution by returning 0 from main.
}