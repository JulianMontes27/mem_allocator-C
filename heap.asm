; Assembly file defining the memory space for our custom allocator
; This file reserves a large, uninitialized block of memory to be used as our heap

bits 32                           ; Specify 32-bit code generation
global memspace                   ; Export the 'memspace' symbol for use in C code

; Define heap size as 1MB in 4-byte words (262144 words)
%define Heapsize (1024 * 1024 / 4)  

; .bss section is for uninitialized memory - perfect for our heap
; Memory in the .bss section is reserved by the linker but is not part of the executable image.
; The operating system's loader will zero-initialize this memory when the program starts.
; This makes it suitable for our heap, which we manage ourselves.
; The OS uses demand paging, so physical memory is only allocated as the program accesses it.
section .bss
align 4                           ; Ensure memory is aligned on 4-byte boundaries for efficient access
memspace:                         ; Label marking the start of our heap
    resd Heapsize                 ; Reserve space for Heapsize double words (4 bytes each)
                                  ; resd = reserve double word

; Mark the stack as non-executable for security
; This is a GNU-specific section that marks the stack as non-executable
; to help prevent buffer overflow exploits
section .note.GNU-stack noalloc noexec nowrite progbits
