#define _GNU_SOURCE 
#include<stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

// Compiler attributes for optimizing code
#define packed __attribute__((__packed__)) // Ensures structure has no padding between fields
#define unused __attribute__((__unused__)) // Silences compiler warnings for unused parameters

// Define maximum heap size: just under 1GB of 4-byte words
// Subtracting 1 word to leave room for bookkeeping/alignment
#define Maxwords ((1024 * 1024 * 1024 / 4) - 1) // Just under 1 GB worth of 4-byte words

// Error codes used by the allocator
#define ErrNoMem 1   // Out of memory error
#define ErrUnknown 2 // Generic/unknown error

// Type definitions for clarity and portability
typedef unsigned char int8;
typedef unsigned short int int16;
typedef unsigned int int32;
typedef unsigned long long int int64;
typedef void heap;
typedef int32 word; // A "word" is our basic 4-byte allocation unit

// Header format: 30 bits size, 1 bit allocated flag, 1 bit reserved
// This compact header takes exactly 4 bytes (one word) with bitfields
struct packed s_header
{
    word w : 30;       // Size of block in 4-byte words (30 bits allows ~1GB addressing)
    bool alloced : 1;  // Flag indicating if block is allocated (1) or free (0)
    bool reserved : 1; // Reserved bit (unused in this implementation)
};

typedef struct packed s_header header;

// Helper macro to set error and return NULL
// The do-while(0) is a common C idiom for safe multi-statement macros
#define reterr(x)    \
    do               \
    {                \
        errno = (x); \
        return NULL; \
    } while (0)

// Helper macro to start searching from beginning of memspace
// Simplifies the calling of findBlock_ by automatically providing the starting address
#define findBlock(x, y, z) findBlock_((header *)(memspace), (x), (y))

// External 1 GB static memory block defined in heap.asm
// This is our heap that the allocator will manage
extern char memspace[];

// Function declarations
// The core memory allocation functions
header *findBlock_(header *, word, word); // Finds a free block of suitable size
void *mkalloc(word, header *);            // Marks a block as allocated
void *alloc(int32);                       // Top-level allocation function (similar to malloc)

// Program entry point
int main(int, char **);