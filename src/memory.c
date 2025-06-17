#include "memory.h"

#include "utils.h"

// Memory size constants
#define MEMORY_SIZE 65536
#define MEMORY_SIZE_MASK (MEMORY_SIZE - 1)

// Memory array
static unsigned char MEMORY[MEMORY_SIZE];

int Memory_Init(void) {
    return 1;
}

unsigned char Memory_GetByte(unsigned short address) {
    return MEMORY[address & MEMORY_SIZE_MASK];
}

void Memory_SetByte(unsigned short address, unsigned char value) {
    MEMORY[address & MEMORY_SIZE_MASK] = value;
}

unsigned short Memory_GetShort(unsigned short address) {
    return TO_SHORT(Memory_GetByte(address), Memory_GetByte(address + 1));
}

void Memory_SetShort(unsigned short address, unsigned short value) {
    Memory_SetByte(address, SHORT_LO(value));
    Memory_SetByte(address + 1, SHORT_HI(value));
}