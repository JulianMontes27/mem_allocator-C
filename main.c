/* main.c - Fixed version */
#include "main.h"

/* Tries to find a free block of memory big enough to hold 'bytes' amount of data and return a pointer to it. Works like regular malloc()
 */
void *mkalloc();


void *alloc(int32 bytes) {
    // Declare pointers as NULL
    header *hdr = NULL;
    void *mem = NULL;

    word words;
    // Convert bytes to # of words
    if (!(bytes % 4)){
        words = bytes/4;
    } else{
        words = (bytes/4) + 1;
    }

    // Assign the address of the first element of the memspace array to the pointer
    mem = memspace;
    hdr = (header *)mem;

    hdr -> w = 1;

    if (!(hdr->w)){
        // If this is the first allocation, and we know the number of words we have, we want to check if there is room for words in the heap
        if (words > Maxwords){
            reterr ErrNoMem;
        }

        mem = mkalloc(words, hdr);

        if(!mem){
            return errno;
        }

        return mem;

    } else {
        // Otherwise, make the allocation
    }

    return $v 0;
}

int main(int unused argc, char **unused argv)
{
    alloc(7);

    return 0;
}
