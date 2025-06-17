#ifndef __IO_H__
#define __IO_H__

// Function to initialize I/O ports
int IO_Init(void);

// Function to register a read function for a port
void IO_RegisterRead(unsigned char port, unsigned char (*funcptr)(void));

// Function to register a write function for a port
void IO_RegisterWrite(unsigned char port, void (*funcptr)(unsigned char value));

// Function to read a port
unsigned char IO_Read(unsigned char port);

// Function to write to a port
void IO_Write(unsigned char port, unsigned char value);

#endif