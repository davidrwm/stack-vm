#include "disk.h"

#include "memory.h"

/*
    Disk size constants
*/

#define DISK_SIZE 16384
#define DISK_SIZE_MASK (DISK_SIZE - 1)
#define DISK_SIZE_SHIFT 14

/*
    Disk count constants
*/

#define DISK_COUNT 1
#define DISK_COUNT_MASK (DISK_COUNT - 1)
#define DISK_COUNT_SHIFT 0

/*
    Disk sector constants
*/

#define DISK_SECTOR_SIZE 256
#define DISK_SECTOR_SIZE_MASK (DISK_SECTOR_SIZE - 1)
#define DISK_SECTOR_SIZE_SHIFT 8

// Macro to get byte from disk
#define DISK_GETBYTE(addr) DISK[((addr) & DISK_SIZE_MASK)]

// Macro to set byte in disk
#define DISK_SETBYTE(addr, byte) DISK[((addr) & DISK_SIZE_MASK)] = (byte)

// Disk data array
static unsigned char DISK[DISK_SIZE];

// Disk struct
static struct {
    // Memory address buffer and data ports
    union {
        struct {
            unsigned char lo;
            unsigned char hi;
        };

        unsigned short value;
    } memoryAddress, data;

    // Status port
    union {
        struct {
            unsigned char intEnable:1; // 0 - Interrupts disabled, 1 - interrupts enabled
            unsigned char ready:1; // 0 - Disk busy, 1 - Disk ready
            unsigned char operation:1; // 0 - Read, 1 - Write
        };

        unsigned char value;
    } status;

    // Disk address for starting sector
    unsigned short diskAddress;

    // Sector count
    unsigned char sectorCount;

    // Number of bytes to read / write
    unsigned byteCount;
} disk;

// Disk command port write function
static void Disk_CommandPortWrite(unsigned char value) {
    switch (value) {
        case DISK_COMMAND_ENABLE_INTERRUPTS:
            disk.status.intEnable = 1;
            break;

        case DISK_COMMAND_DISABLE_INTERRUPTS:
            disk.status.intEnable = 0;
            break;

        case DISK_COMMAND_GET_DISK_NUMBER:
            disk.data.lo = 0;
            disk.data.hi = 0;

            break;

        case DISK_COMMAND_SET_START_SECTOR:
            disk.diskAddress = disk.data.lo << DISK_SECTOR_SIZE_SHIFT;
            break;

        case DISK_COMMAND_SET_MEMORY_ADDRESS:
            disk.memoryAddress.value = disk.data.value;
            break;

        case DISK_COMMAND_SET_SECTOR_COUNT:
            disk.sectorCount = disk.data.lo;
            break;

        case DISK_COMMAND_READ_SECTORS:
            disk.status.operation = DISK_OPERATION_READ;
            disk.byteCount = disk.sectorCount << DISK_SECTOR_SIZE_SHIFT;
            break;

        case DISK_COMMAND_WRITE_SECTORS:
            disk.status.operation = DISK_OPERATION_WRITE;
            disk.byteCount = disk.sectorCount << DISK_SECTOR_SIZE_SHIFT;
            break;

        default:
            break;
    }
}

// Disk data port functions

static unsigned char Disk_DataLoPortRead(void) { return disk.data.lo; }
static unsigned char Disk_DataHiPortRead(void) { return disk.data.hi; }

static void Disk_DataLoPortWrite(unsigned char value) { disk.data.lo = value; }
static void Disk_DataHiPortWrite(unsigned char value) { disk.data.hi = value; }

// Disk status port read function
static unsigned char Disk_StatusPortRead(void) { return disk.status.value; }

// Function to read bytes from disk to memory
static void Disk_ReadByte(void) {
    disk.status.ready = !disk.byteCount;

    Memory_SetByte(disk.memoryAddress.value++, DISK_GETBYTE(disk.diskAddress++));

    disk.byteCount--;
}

// Function to write sectors from memory to disk
static void Disk_WriteByte(void) {
    disk.status.ready = !disk.byteCount;

    DISK_SETBYTE(disk.diskAddress++, Memory_GetByte(disk.memoryAddress.value++));

    disk.byteCount--;
}

// Disk operation function pointer array
static void (*Disk_Operation[2])(void) = {
    Disk_ReadByte,
    Disk_WriteByte
};

int Disk_Init(void) {
    // Disable interrupts and ready the disk
    disk.status.intEnable = 0;
    disk.status.ready = 1;
    
    return 1;
}

void Disk_Quit(void) {
    return;
}

int Disk_LoadImage(const char *path) {
    return 1;
}

void Disk_Update(void) {
    // Return if the disk is not busy (reading or writing)
    if (disk.status.ready) return;

    // Perform the current disk operation
    Disk_Operation[disk.status.operation]();
}