; heap.asm
bits 32
; global directive to make the symbol visible to the linker (ld)
global memspace

section .data
memspace:
    db 'A', 'B', 'C', 'D', 0   ; Initialize memory with some values for testing

section .note.GNU-stack noalloc noexec nowrite progbits
