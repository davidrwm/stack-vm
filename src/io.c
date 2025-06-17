#include "io.h"

// Read function pointers array
static unsigned char (*IO_READ_FUNPTR[256])(void) = { 0 };

// Write function pointers array
static void (*IO_WRITE_FUNPTR[256])(unsigned char value) = { 0 };

// Default read function
static unsigned char IO_READ_DEFAULT(void) { return 0; }

// Default write function
static void IO_WRITE_DEFAULT(unsigned char value) { return; }

int IO_Init(void) {
    for (int i = 0; i < 256; i++) {
        IO_READ_FUNPTR[i] = IO_READ_DEFAULT;
        IO_WRITE_FUNPTR[i] = IO_WRITE_DEFAULT;
    }

    return 1;
}

void IO_RegisterRead(unsigned char port, unsigned char (*funcptr)(void)) {
    IO_READ_FUNPTR[port] = funcptr;
}

void IO_RegisterWrite(unsigned char port, void (*funcptr)(unsigned char value)) {
    IO_WRITE_FUNPTR[port] = funcptr;
}

unsigned char IO_Read(unsigned char port) {
    return IO_READ_FUNPTR[port]();
}

void IO_Write(unsigned char port, unsigned char value) {
    IO_WRITE_FUNPTR[port](value);
}