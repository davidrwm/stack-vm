#ifndef __MEMORY_H__
#define __MEMORY_H__

// Function to initialize memory
int Memory_Init(void);

// Function to get a byte from memory
unsigned char Memory_GetByte(unsigned short address);

// Function to set a byte in memory
void Memory_SetByte(unsigned short address, unsigned char value);

// Function to get a short from memory
unsigned short Memory_GetShort(unsigned short address);

// Function to set a short in memory
void Memory_SetShort(unsigned short address, unsigned short value);

#endif