#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

// Compiler attributes for optimizing code and providing metadata
#define packed __attribute__((__packed__)) // Ensures structure has no padding between fields. Important for bitfields to occupy exact size.
#define unused __attribute__((__unused__)) // Silences compiler warnings for unused parameters. Useful in main function signature for unused argc/argv.

// Define maximum heap size: just under 1MB of 4-byte words
// We define the size in terms of 4-byte words because our allocator operates on word-sized units.
// Maxwords represents the total capacity of our heap area in words.
#define Maxwords ((1024 * 1024 / 4) - 1) // 1 MB is 1024*1024 bytes. Divided by 4 gives words. Subtracting 1 word might be for alignment or just a design choice.

// Error codes used by the allocator
#define ErrNoMem 1 // Indicates that an allocation request could not be fulfilled due to lack of available memory.
#define ErrUnknown 2 // A generic or unhandled error condition.

// Type definitions for clarity and portability
typedef unsigned char int8;           // 8-bit unsigned integer
typedef unsigned short int int16;     // 16-bit unsigned integer
typedef unsigned int int32;           // 32-bit unsigned integer
typedef unsigned long long int int64; // 64-bit unsigned integer
typedef void heap;                    // Type to represent the abstract heap memory area. Used as a pointer type.
typedef int32 word;                   // A "word" is our basic 4-byte allocation unit. Defined as int32 for clarity.

// Header format: 30 bits size, 1 bit allocated flag, 1 bit reserved
// This structure defines the metadata for each block of memory in the heap.
// Using bitfields allows us to pack this information efficiently into a single 4-byte word.
struct packed s_header
{
    word w : 30; // Size of the block *including* the header itself, in 4-byte words. 30 bits allows for a maximum size of 2^30 words, which is sufficient for a 1GB heap.

    bool alloced : 1; // Flag indicating if the block is currently allocated (true/1) or free (false/0).

    bool reserved : 1; // A reserved bit, currently unused. Could be used for future features like boundary tags.
};

typedef struct packed s_header header; // Alias the struct type to 'header' for convenience.

// Helper macro to set the global 'errno' variable and return NULL
// This is a common pattern for functions that indicate failure by returning NULL and setting errno.
// The do-while(0) loop makes the macro behave like a single statement, preventing issues in if/else blocks without braces.
#define reterr(x)    \
    do               \
    {                \
        errno = (x); \
        return NULL; \
    } while (0)

// Helper macro to start searching for a free block from the beginning of the memspace.
// Simplifies the initial call to findBlock_ by providing the starting address and offset.
// FIX: Corrected the parameter order in the macro expansion.
#define findBlock(words_to_alloc, start_offset) findBlock_((header *)(memspace), (words_to_alloc), (start_offset))

// External 1 MB static memory block defined in heap.asm
// This is the raw memory area that our allocator will manage.
// Declared as 'extern char' to get a byte pointer to the start of the memory block.
extern char memspace[];

// Function declarations
// The core memory allocation functions provided by this allocator.
header *findBlock_(header *hdr, word words_to_alloc, word n); // Finds a free block of suitable size
void freealloc(void *ptr);
void *mkalloc(word words_to_alloc, header *hdr);              // Marks a block as allocated
void *alloc(int32 bytes);                                     // Top-level allocation function (similar to malloc)

// Helper function to print the current state of the memory layout (blocks and their status).
void print_memory_layout();

// Program entry point
int main(int, char **);