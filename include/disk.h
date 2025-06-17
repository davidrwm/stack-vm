#ifndef __DISK_H__
#define __DISK_H__

// Disk port enums
typedef enum disk_port_e {
    DISK_PORT_COMMAND = 0x20,
    DISK_PORT_DATA_LO,
    DISK_PORT_DATA_HI,
    DISK_PORT_STATUS,
} DiskPort;

// Disk command enums
typedef enum disk_command_e {
    DISK_COMMAND_ENABLE_INTERRUPTS = 0x00,
    DISK_COMMAND_DISABLE_INTERRUPTS,
    DISK_COMMAND_GET_DISK_NUMBER,
    DISK_COMMAND_SET_START_SECTOR,
    DISK_COMMAND_SET_MEMORY_ADDRESS,
    DISK_COMMAND_SET_SECTOR_COUNT,
    DISK_COMMAND_READ_SECTORS,
    DISK_COMMAND_WRITE_SECTORS,
} DiskCommand;

// Disk operation enum
typedef enum disk_operation_e {
    DISK_OPERATION_READ,
    DISK_OPERATION_WRITE,
} DiskOperation;

// Function to initialize the disk
int Disk_Init(void);

// Function to quit the disk
void Disk_Quit(void);

// Function to load a disk image
int Disk_LoadImage(const char *path);

// Function to update the disk
void Disk_Update(void);

#endif