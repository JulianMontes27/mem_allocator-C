CC = gcc
ASM = nasm
CFLAGS = -Wall -Wextra -g -m32 -O2
ASMFLAGS = -f elf32

TARGET = memory_app
OBJECTS = main.o heap.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c main.h
	$(CC) $(CFLAGS) -c -o $@ $<

heap.o: heap.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

clean:
	rm -f $(TARGET) $(OBJECTS)