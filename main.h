/* main.h */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

/* Tell the compiler how to manage different events and/or errors */
#define packed __attribute__((__packed__))
#define unused __attribute__((__unused))
#define Maxwords ((1024*1024*1024/4) - 1)

#define ErrNoMem 1

typedef unsigned char int8;
typedef unsigned short int int16;
typedef unsigned int int32;
typedef unsigned long long int int64;
typedef void heap;
typedef int32 word;

/* 32 bit (4 byte words) */
struct packed s_header {
    word w: 30;
    bool alloced: 1;
    bool unused reserved: 1;
};

typedef struct packed s_header header;

#define $1 (int8 *)
#define $2 (int16)
#define $3 (int32)
#define $8 (int64)
#define $16 (int128)
#define $c (char *)
#define $i (int)
#define $v (void *)
#define $h (header *)

#define reterr(x) errno = (x); return $v 0

/* Declare the external memspace variable correctly as an array */
extern char memspace[]; /* An array has a proper address, not a null pointer */

/* Declare the alloc() function, that takes 'n' bytes to allocate on the heap's (memspace) contiguous memory at runtime; returns a void pointer, which has to be converted to the required type to be used, to the first address. If allocation fails, return NULL pointer*/
/* For compliance with universal computer architectures, users may want to use sizeof to get the real size of a data type to store on the heap */
void *alloc(int32);
void *mkalloc(word, header*);
int main(int, char **);