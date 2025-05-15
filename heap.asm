; Assembly file defining the memory space for our custom allocator
; This file reserves a large, uninitialized block of memory to be used as our heap

bits 32                           ; Specify 32-bit code generation
global memspace                   ; Export the 'memspace' symbol for use in C code

; Define heap size as 1GB in 4-byte words (268,435,456 words)
%define Heapsize (1024 * 1024 * 1024 / 4)  

; .bss section is for uninitialized memory - perfect for our heap
; The OS will reserve this space but won't actually allocate physical memory
; until the program tries to use it (demand paging)
section .bss
align 4                           ; Ensure memory is aligned on 4-byte boundaries for efficient access
memspace:                         ; Label marking the start of our heap
    resd Heapsize                 ; Reserve space for Heapsize double words (4 bytes each)
                                  ; resd = reserve double word

; Mark the stack as non-executable for security
; This is a GNU-specific section that marks the stack as non-executable
; to help prevent buffer overflow exploits
section .note.GNU-stack noalloc noexec nowrite progbits