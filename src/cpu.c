#include "cpu.h"

#include "memory.h"
#include "io.h"
#include "utils.h"

// The CPU struct

static struct {
    // 16 - bit CPU registers
    union {
        struct {
            unsigned char l;
            unsigned char h;
        };

        unsigned short value;
    } a, b, s, i;

    // Flags register
    union {
        struct {
            unsigned char z:1; // Zero flag
            unsigned char c:1; // Carry flag
            unsigned char s:1; // Sign flag
            unsigned char v:1; // Overflow flag
            unsigned char h:1; // Halt flag
            unsigned char i:1; // Interrupt enable flag
        };

        unsigned char value;
    } f;
} cpu;

// Helper function to fetch a byte
static unsigned char CPU_FetchByte(void) {
    return Memory_GetByte(cpu.i.value++);
}

// Helper function to fetch a short
static unsigned short CPU_FetchShort(void) {
    unsigned char lo = CPU_FetchByte();
    unsigned char hi = CPU_FetchByte();

    return TO_SHORT(lo, hi);
}

// Helper function to push a byte onto the stack
static void CPU_PushByte(unsigned char value) {
    Memory_SetByte(--cpu.s.value, value);
}

// Helper function to pop a byte from the stack
static unsigned char CPU_PopByte(void) {
    return Memory_GetByte(cpu.s.value++);
}

// Helper function to push a short onto the stack
static void CPU_PushShort(unsigned short value) {
    CPU_PushByte(SHORT_HI(value));
    CPU_PushByte(SHORT_LO(value));
}

// Helper function to pop a short from the stack
static unsigned short CPU_PopShort(void) {
    unsigned char lo = CPU_PopByte();
    unsigned char hi = CPU_PopByte();

    return TO_SHORT(lo, hi);
}

// Helper macro to get the effective address for addressing mode "D"
#define CPU_ADDRESS_D CPU_FetchShort()

// Helper macros to get the effective address for addressing mode "R"
#define CPU_ADDRESS_RA cpu.a.value
#define CPU_ADDRESS_RB cpu.b.value

// Helper macros to get the effective address for addressing mode "X"
#define CPU_ADDRESS_XA (cpu.a.value + CPU_FetchShort())
#define CPU_ADDRESS_XB (cpu.b.value + CPU_FetchShort())

// Helper macros to get the effective address for addressing mode "Y"
#define CPU_ADDRESS_YA Memory_GetShort(cpu.a.value + CPU_FetchShort())
#define CPU_ADDRESS_YB Memory_GetShort(cpu.b.value + CPU_FetchShort())

/*
    8 - bit ALU helper functions
*/

static unsigned char CPU_ADB(unsigned char a, unsigned char b) {
    unsigned short result = a + b + cpu.f.c;
    unsigned char resultByte = result;

    // Update status flags
    cpu.f.z = !resultByte;
    cpu.f.c = result > 0xFF;
    cpu.f.s = resultByte >> 7;
    cpu.f.v = 0;

    return resultByte;
}

static unsigned char CPU_SUB(unsigned char a, unsigned char b) {
    return CPU_ADB(a, ~b);
}

static unsigned char CPU_ANB(unsigned char a, unsigned char b) {
    unsigned char resultByte = a & b;

    // Update status flags
    cpu.f.z = !resultByte;
    cpu.f.c = 0;
    cpu.f.s = resultByte >> 7;
    cpu.f.v = 0;

    return resultByte;
}

static unsigned char CPU_ORB(unsigned char a, unsigned char b) {
    unsigned char resultByte = a | b;

    // Update status flags
    cpu.f.z = !resultByte;
    cpu.f.c = 0;
    cpu.f.s = resultByte >> 7;
    cpu.f.v = 0;

    return resultByte;
}

static unsigned char CPU_XRB(unsigned char a, unsigned char b) {
    unsigned char resultByte = a ^ b;

    // Update status flags
    cpu.f.z = !resultByte;
    cpu.f.c = 0;
    cpu.f.s = resultByte >> 7;
    cpu.f.v = 0;

    return resultByte;
}

static unsigned char CPU_IVB(unsigned char a) {
    unsigned char resultByte = ~a;

    // Update status flags
    cpu.f.z = !resultByte;
    cpu.f.c = 0;
    cpu.f.s = resultByte >> 7;
    cpu.f.v = 0;

    return resultByte;
}

static unsigned char CPU_RLB(unsigned char a) {
    unsigned char resultByte = (a << 1) | cpu.f.c;

    // Update status flags
    cpu.f.z = !resultByte;
    cpu.f.c = a >> 7;
    cpu.f.s = resultByte >> 7;
    cpu.f.v = 0;

    return resultByte;
}

static unsigned char CPU_RRB(unsigned char a) {
    unsigned char resultByte = (a >> 1) | (cpu.f.c << 7);

    // Update status flags
    cpu.f.z = !resultByte;
    cpu.f.c = a & 1;
    cpu.f.s = resultByte >> 7;
    cpu.f.v = 0;

    return resultByte;
}

/*
    16 - bit ALU helper functions
*/

static unsigned short CPU_ADS(unsigned short a, unsigned short b) {
    unsigned result = a + b + cpu.f.c;
    unsigned short resultShort = result;

    // Update status flags
    cpu.f.z = !resultShort;
    cpu.f.c = result > 0xFFFF;
    cpu.f.s = resultShort >> 15;
    cpu.f.v = 0;

    return resultShort;
}

static unsigned short CPU_SUS(unsigned short a, unsigned short b) {
    return CPU_ADS(a, ~b);
}

static unsigned short CPU_ANS(unsigned short a, unsigned short b) {
    unsigned short resultShort = a & b;

    // Update status flags
    cpu.f.z = !resultShort;
    cpu.f.c = 0;
    cpu.f.s = resultShort >> 15;
    cpu.f.v = 0;

    return resultShort;
}

static unsigned short CPU_ORS(unsigned short a, unsigned short b) {
    unsigned short resultShort = a | b;

    // Update status flags
    cpu.f.z = !resultShort;
    cpu.f.c = 0;
    cpu.f.s = resultShort >> 15;
    cpu.f.v = 0;

    return resultShort;
}

static unsigned short CPU_XRS(unsigned short a, unsigned short b) {
    unsigned short resultShort = a ^ b;

    // Update status flags
    cpu.f.z = !resultShort;
    cpu.f.c = 0;
    cpu.f.s = resultShort >> 15;
    cpu.f.v = 0;

    return resultShort;
}

static unsigned short CPU_IVS(unsigned short a) {
    unsigned short resultShort = ~a;

    // Update status flags
    cpu.f.z = !resultShort;
    cpu.f.c = 0;
    cpu.f.s = resultShort >> 15;
    cpu.f.v = 0;

    return resultShort;
}

static unsigned short CPU_RLS(unsigned short a) {
    unsigned short resultShort = (a << 1) | cpu.f.c;

    // Update status flags
    cpu.f.z = !resultShort;
    cpu.f.c = a >> 15;
    cpu.f.s = resultShort >> 15;
    cpu.f.v = 0;

    return resultShort;
}

static unsigned short CPU_RRS(unsigned short a) {
    unsigned short resultShort = (a >> 1) | (cpu.f.c << 7);

    // Update status flags
    cpu.f.z = !resultShort;
    cpu.f.c = a & 1;
    cpu.f.s = resultShort >> 15;
    cpu.f.v = 0;

    return resultShort;
}

/*
    Load instructions
*/

static void CPU_Opcode_LDAI(void) { cpu.a.value = CPU_FetchShort(); }
static void CPU_Opcode_LDBI(void) { cpu.b.value = CPU_FetchShort(); }
static void CPU_Opcode_LDSI(void) { cpu.s.value = CPU_FetchShort(); }

static void CPU_Opcode_LDAD(void) { cpu.a.value = Memory_GetShort(CPU_ADDRESS_D); }
static void CPU_Opcode_LDBD(void) { cpu.b.value = Memory_GetShort(CPU_ADDRESS_D); }
static void CPU_Opcode_LDSD(void) { cpu.s.value = Memory_GetShort(CPU_ADDRESS_D); }

static void CPU_Opcode_LDARA(void) { cpu.a.value = Memory_GetShort(CPU_ADDRESS_RA); }
static void CPU_Opcode_LDBRA(void) { cpu.b.value = Memory_GetShort(CPU_ADDRESS_RA); }
static void CPU_Opcode_LDSRA(void) { cpu.s.value = Memory_GetShort(CPU_ADDRESS_RA); }

static void CPU_Opcode_LDARB(void) { cpu.a.value = Memory_GetShort(CPU_ADDRESS_RB); }
static void CPU_Opcode_LDBRB(void) { cpu.b.value = Memory_GetShort(CPU_ADDRESS_RB); }
static void CPU_Opcode_LDSRB(void) { cpu.s.value = Memory_GetShort(CPU_ADDRESS_RB); }

static void CPU_Opcode_LDAXA(void) { cpu.a.value = Memory_GetShort(CPU_ADDRESS_XA); }
static void CPU_Opcode_LDBXA(void) { cpu.b.value = Memory_GetShort(CPU_ADDRESS_XA); }
static void CPU_Opcode_LDSXA(void) { cpu.s.value = Memory_GetShort(CPU_ADDRESS_XA); }

static void CPU_Opcode_LDAXB(void) { cpu.a.value = Memory_GetShort(CPU_ADDRESS_XB); }
static void CPU_Opcode_LDBXB(void) { cpu.b.value = Memory_GetShort(CPU_ADDRESS_XB); }
static void CPU_Opcode_LDSXB(void) { cpu.s.value = Memory_GetShort(CPU_ADDRESS_XB); }

static void CPU_Opcode_LDAYA(void) { cpu.a.value = Memory_GetShort(CPU_ADDRESS_YA); }
static void CPU_Opcode_LDBYA(void) { cpu.b.value = Memory_GetShort(CPU_ADDRESS_YA); }
static void CPU_Opcode_LDSYA(void) { cpu.s.value = Memory_GetShort(CPU_ADDRESS_YA); }

static void CPU_Opcode_LDAYB(void) { cpu.a.value = Memory_GetShort(CPU_ADDRESS_YB); }
static void CPU_Opcode_LDBYB(void) { cpu.b.value = Memory_GetShort(CPU_ADDRESS_YB); }
static void CPU_Opcode_LDSYB(void) { cpu.s.value = Memory_GetShort(CPU_ADDRESS_YB); }

/*
    Store instructions
*/

static void CPU_Opcode_STAD(void) { Memory_SetShort(CPU_FetchShort(), cpu.a.value); }
static void CPU_Opcode_STBD(void) { Memory_SetShort(CPU_FetchShort(), cpu.b.value); }
static void CPU_Opcode_STSD(void) { Memory_SetShort(CPU_FetchShort(), cpu.s.value); }

static void CPU_Opcode_STARA(void) { Memory_SetShort(CPU_ADDRESS_RA, cpu.a.value); }
static void CPU_Opcode_STBRA(void) { Memory_SetShort(CPU_ADDRESS_RA, cpu.b.value); }
static void CPU_Opcode_STSRA(void) { Memory_SetShort(CPU_ADDRESS_RA, cpu.s.value); }

static void CPU_Opcode_STARB(void) { Memory_SetShort(CPU_ADDRESS_RB, cpu.a.value); }
static void CPU_Opcode_STBRB(void) { Memory_SetShort(CPU_ADDRESS_RB, cpu.b.value); }
static void CPU_Opcode_STSRB(void) { Memory_SetShort(CPU_ADDRESS_RB, cpu.s.value); }

static void CPU_Opcode_STAXA(void) { Memory_SetShort(CPU_ADDRESS_XA, cpu.a.value); }
static void CPU_Opcode_STBXA(void) { Memory_SetShort(CPU_ADDRESS_XA, cpu.b.value); }
static void CPU_Opcode_STSXA(void) { Memory_SetShort(CPU_ADDRESS_XA, cpu.s.value); }

static void CPU_Opcode_STAXB(void) { Memory_SetShort(CPU_ADDRESS_XB, cpu.a.value); }
static void CPU_Opcode_STBXB(void) { Memory_SetShort(CPU_ADDRESS_XB, cpu.b.value); }
static void CPU_Opcode_STSXB(void) { Memory_SetShort(CPU_ADDRESS_XB, cpu.s.value); }

static void CPU_Opcode_STAYA(void) { Memory_SetShort(CPU_ADDRESS_YA, cpu.a.value); }
static void CPU_Opcode_STBYA(void) { Memory_SetShort(CPU_ADDRESS_YA, cpu.b.value); }
static void CPU_Opcode_STSYA(void) { Memory_SetShort(CPU_ADDRESS_YA, cpu.s.value); }

static void CPU_Opcode_STAYB(void) { Memory_SetShort(CPU_ADDRESS_YB, cpu.a.value); }
static void CPU_Opcode_STBYB(void) { Memory_SetShort(CPU_ADDRESS_YB, cpu.b.value); }
static void CPU_Opcode_STSYB(void) { Memory_SetShort(CPU_ADDRESS_YB, cpu.s.value); }

/*
    Move instructions
*/

static void CPU_Opcode_MVAB(void) { cpu.a.value = cpu.b.value; }
static void CPU_Opcode_MVAS(void) { cpu.a.value = cpu.s.value; }
static void CPU_Opcode_MVAI(void) { cpu.a.value = cpu.i.value; }

static void CPU_Opcode_MVBA(void) { cpu.b.value = cpu.a.value; }
static void CPU_Opcode_MVBS(void) { cpu.b.value = cpu.s.value; }
static void CPU_Opcode_MVBI(void) { cpu.b.value = cpu.i.value; }

static void CPU_Opcode_MVSA(void) { cpu.s.value = cpu.a.value; }
static void CPU_Opcode_MVSB(void) { cpu.s.value = cpu.b.value; }
static void CPU_Opcode_MVSI(void) { cpu.s.value = cpu.i.value; }

static void CPU_Opcode_MVIA(void) { cpu.i.value = cpu.a.value; }
static void CPU_Opcode_MVIB(void) { cpu.i.value = cpu.b.value; }
static void CPU_Opcode_MVIS(void) { cpu.i.value = cpu.s.value; }

/*
    Push instructions
*/

static void CPU_Opcode_PUBI(void) { CPU_PushByte(CPU_FetchByte()); }
static void CPU_Opcode_PUBD(void) { CPU_PushByte(Memory_GetByte(CPU_ADDRESS_D)); }
static void CPU_Opcode_PUBRA(void) { CPU_PushByte(Memory_GetByte(CPU_ADDRESS_RA)); }
static void CPU_Opcode_PUBRB(void) { CPU_PushByte(Memory_GetByte(CPU_ADDRESS_RB)); }
static void CPU_Opcode_PUBXA(void) { CPU_PushByte(Memory_GetByte(CPU_ADDRESS_XA)); }
static void CPU_Opcode_PUBXB(void) { CPU_PushByte(Memory_GetByte(CPU_ADDRESS_XB)); }
static void CPU_Opcode_PUBYA(void) { CPU_PushByte(Memory_GetByte(CPU_ADDRESS_YA)); }
static void CPU_Opcode_PUBYB(void) { CPU_PushByte(Memory_GetByte(CPU_ADDRESS_YB)); }

static void CPU_Opcode_PUSI(void) { CPU_PushShort(CPU_FetchShort()); }
static void CPU_Opcode_PUSD(void) { CPU_PushShort(Memory_GetShort(CPU_ADDRESS_D)); }
static void CPU_Opcode_PUSRA(void) { CPU_PushShort(Memory_GetShort(CPU_ADDRESS_RA)); }
static void CPU_Opcode_PUSRB(void) { CPU_PushShort(Memory_GetShort(CPU_ADDRESS_RB)); }
static void CPU_Opcode_PUSXA(void) { CPU_PushShort(Memory_GetShort(CPU_ADDRESS_XA)); }
static void CPU_Opcode_PUSXB(void) { CPU_PushShort(Memory_GetShort(CPU_ADDRESS_XB)); }
static void CPU_Opcode_PUSYA(void) { CPU_PushShort(Memory_GetShort(CPU_ADDRESS_YA)); }
static void CPU_Opcode_PUSYB(void) { CPU_PushShort(Memory_GetShort(CPU_ADDRESS_YB)); }

static void CPU_Opcode_PUA(void) { CPU_PushShort(cpu.a.value); }
static void CPU_Opcode_PUB(void) { CPU_PushShort(cpu.b.value); }
static void CPU_Opcode_PUS(void) { CPU_PushShort(cpu.s.value); }
static void CPU_Opcode_PUI(void) { CPU_PushShort(cpu.i.value); }
static void CPU_Opcode_PUF(void) { CPU_PushByte(cpu.f.value); }

/*
    Pop instructions
*/

static void CPU_Opcode_POBD(void) { Memory_SetByte(CPU_ADDRESS_D, CPU_PopByte()); }
static void CPU_Opcode_POBRA(void) { Memory_SetByte(CPU_ADDRESS_RA, CPU_PopByte()); }
static void CPU_Opcode_POBRB(void) { Memory_SetByte(CPU_ADDRESS_RB, CPU_PopByte()); }
static void CPU_Opcode_POBXA(void) { Memory_SetByte(CPU_ADDRESS_XA, CPU_PopByte()); }
static void CPU_Opcode_POBXB(void) { Memory_SetByte(CPU_ADDRESS_XB, CPU_PopByte()); }
static void CPU_Opcode_POBYA(void) { Memory_SetByte(CPU_ADDRESS_YA, CPU_PopByte()); }
static void CPU_Opcode_POBYB(void) { Memory_SetByte(CPU_ADDRESS_YB, CPU_PopByte()); }

static void CPU_Opcode_POSD(void) { Memory_SetShort(CPU_ADDRESS_D, CPU_PopShort()); }
static void CPU_Opcode_POSRA(void) { Memory_SetShort(CPU_ADDRESS_RA, CPU_PopShort()); }
static void CPU_Opcode_POSRB(void) { Memory_SetShort(CPU_ADDRESS_RB, CPU_PopShort()); }
static void CPU_Opcode_POSXA(void) { Memory_SetShort(CPU_ADDRESS_XA, CPU_PopShort()); }
static void CPU_Opcode_POSXB(void) { Memory_SetShort(CPU_ADDRESS_XB, CPU_PopShort()); }
static void CPU_Opcode_POSYA(void) { Memory_SetShort(CPU_ADDRESS_YA, CPU_PopShort()); }
static void CPU_Opcode_POSYB(void) { Memory_SetShort(CPU_ADDRESS_YB, CPU_PopShort()); }

static void CPU_Opcode_POA(void) { cpu.a.value = CPU_PopShort(); }
static void CPU_Opcode_POB(void) { cpu.b.value = CPU_PopShort(); }
static void CPU_Opcode_POS(void) { cpu.s.value = CPU_PopShort(); }
static void CPU_Opcode_POI(void) { cpu.i.value = CPU_PopShort(); }
static void CPU_Opcode_POF(void) { cpu.f.value = CPU_PopByte(); }

/*
    Stack instructions
*/

static void CPU_Opcode_DTS(void) { CPU_PushByte(Memory_GetByte(cpu.s.value)); }

static void CPU_Opcode_STS(void) {
    unsigned char top1 = Memory_GetByte(cpu.s.value);
    unsigned char top2 = Memory_GetByte(cpu.s.value + 1);

    Memory_SetByte(cpu.s.value, top2);
    Memory_SetByte(cpu.s.value + 1, top1);
}

/*
    Indexing register instructions
*/

static void CPU_Opcode_IRA(void) { cpu.a.value++; }
static void CPU_Opcode_IRB(void) { cpu.b.value++; }
static void CPU_Opcode_IRS(void) { cpu.s.value++; }

static void CPU_Opcode_DRA(void) { cpu.a.value--; }
static void CPU_Opcode_DRB(void) { cpu.b.value--; }
static void CPU_Opcode_DRS(void) { cpu.s.value--; }

/*
    8 - bit ALU instructions
*/

static void CPU_Opcode_ADB(void) {
    unsigned char a = CPU_PopByte();
    unsigned char b = CPU_PopByte();

    CPU_PushByte(CPU_ADB(a, b));
}

static void CPU_Opcode_SUB(void) {
    unsigned char a = CPU_PopByte();
    unsigned char b = CPU_PopByte();

    CPU_PushByte(CPU_SUB(a, b));
}

static void CPU_Opcode_ANB(void) {
    unsigned char a = CPU_PopByte();
    unsigned char b = CPU_PopByte();

    CPU_PushByte(CPU_ANB(a, b));
}

static void CPU_Opcode_ORB(void) {
    unsigned char a = CPU_PopByte();
    unsigned char b = CPU_PopByte();

    CPU_PushByte(CPU_ORB(a, b));
}

static void CPU_Opcode_XRB(void) {
    unsigned char a = CPU_PopByte();
    unsigned char b = CPU_PopByte();

    CPU_PushByte(CPU_XRB(a, b));
}

static void CPU_Opcode_CPB(void) {
    unsigned char a = CPU_PopByte();
    unsigned char b = CPU_PopByte();

    cpu.f.c = 1;

    CPU_SUB(a, b);
}

static void CPU_Opcode_IVB(void) {
    unsigned char a = CPU_PopByte();

    CPU_PushByte(CPU_IVB(a));
}

static void CPU_Opcode_ICB(void) {
    unsigned char a = CPU_PopByte();

    cpu.f.c = 0;

    CPU_PushByte(CPU_ADB(a, 1));
}

static void CPU_Opcode_DCB(void) {
    unsigned char a = CPU_PopByte();

    cpu.f.c = 1;

    CPU_PushByte(CPU_SUB(a, 1));
}

static void CPU_Opcode_RLB(void) {
    unsigned char a = CPU_PopByte();

    CPU_PushByte(CPU_RLB(a));
}

static void CPU_Opcode_RRB(void) {
    unsigned char a = CPU_PopByte();

    CPU_PushByte(CPU_RRB(a));
}

static void CPU_Opcode_SLB(void) {
    unsigned char a = CPU_PopByte();

    cpu.f.c = 0;

    CPU_PushByte(CPU_RLB(a));
}

static void CPU_Opcode_SRB(void) {
    unsigned char a = CPU_PopByte();

    cpu.f.c = 0;

    CPU_PushByte(CPU_RRB(a));
}

static void CPU_Opcode_SAB(void) {
    unsigned char a = CPU_PopByte();

    cpu.f.c = 1;

    CPU_PushByte(CPU_RRB(a));
}

/*
    16 - bit ALU instructions
*/

static void CPU_Opcode_ADS(void) {
    unsigned short a = CPU_PopShort();
    unsigned short b = CPU_PopShort();

    CPU_PushShort(CPU_ADS(a, b));
}

static void CPU_Opcode_SUS(void) {
    unsigned short a = CPU_PopShort();
    unsigned short b = CPU_PopShort();

    CPU_PushShort(CPU_SUS(a, b));
}

static void CPU_Opcode_ANS(void) {
    unsigned short a = CPU_PopShort();
    unsigned short b = CPU_PopShort();

    CPU_PushShort(CPU_ANS(a, b));
}

static void CPU_Opcode_ORS(void) {
    unsigned short a = CPU_PopShort();
    unsigned short b = CPU_PopShort();

    CPU_PushShort(CPU_ORS(a, b));
}

static void CPU_Opcode_XRS(void) {
    unsigned short a = CPU_PopShort();
    unsigned short b = CPU_PopShort();

    CPU_PushShort(CPU_XRS(a, b));
}

static void CPU_Opcode_CPS(void) {
    unsigned short a = CPU_PopShort();
    unsigned short b = CPU_PopShort();

    cpu.f.c = 1;

    CPU_SUS(a, b);
}

static void CPU_Opcode_IVS(void) {
    unsigned short a = CPU_PopShort();

    CPU_PushShort(CPU_IVS(a));
}

static void CPU_Opcode_ICS(void) {
    unsigned short a = CPU_PopShort();

    cpu.f.c = 0;

    CPU_PushShort(CPU_ADS(a, 1));
}

static void CPU_Opcode_DCS(void) {
    unsigned short a = CPU_PopShort();

    cpu.f.c = 1;

    CPU_PushShort(CPU_SUS(a, 1));
}

static void CPU_Opcode_RLS(void) {
    unsigned short a = CPU_PopShort();

    CPU_PushShort(CPU_RLS(a));
}

static void CPU_Opcode_RRS(void) {
    unsigned short a = CPU_PopShort();

    CPU_PushShort(CPU_RRS(a));
}

static void CPU_Opcode_SLS(void) {
    unsigned short a = CPU_PopShort();

    cpu.f.c = 0;

    CPU_PushShort(CPU_RLS(a));
}

static void CPU_Opcode_SRS(void) {
    unsigned short a = CPU_PopShort();

    cpu.f.c = 0;

    CPU_PushShort(CPU_RRS(a));
}

static void CPU_Opcode_SAS(void) {
    unsigned short a = CPU_PopShort();

    cpu.f.c = 1;

    CPU_PushShort(CPU_RRS(a));
}

/*
    Status flag instructions
*/

static void CPU_Opcode_SFZ(void) { cpu.f.z = 1; }
static void CPU_Opcode_SFC(void) { cpu.f.c = 1; }
static void CPU_Opcode_SFS(void) { cpu.f.s = 1; }
static void CPU_Opcode_SFV(void) { cpu.f.v = 1; }

static void CPU_Opcode_CFZ(void) { cpu.f.z = 0; }
static void CPU_Opcode_CFC(void) { cpu.f.c = 0; }
static void CPU_Opcode_CFS(void) { cpu.f.s = 0; }
static void CPU_Opcode_CFV(void) { cpu.f.v = 0; }

static void CPU_Opcode_EI(void) { cpu.f.i = 1; }
static void CPU_Opcode_DI(void) { cpu.f.i = 0; }

static void CPU_Opcode_HT(void) { cpu.f.h = 1; }

/*
    Branching instructions
*/

// Helper function for calling an address
static void CPU_Util_CA(unsigned short address) {
    CPU_PushShort(cpu.i.value);
    cpu.i.value = address;
}

// Helper function to do a conditional jump
static void CPU_Util_CondJM(unsigned char condition) {
    unsigned short address = CPU_FetchShort();

    if (condition) cpu.i.value = address;
}

// Helper function to do a conditional call
static void CPU_Util_CondCA(unsigned char condition) {
    unsigned short address = CPU_FetchShort();

    if (condition) {
        CPU_PushShort(cpu.i.value);

        cpu.i.value = address;
    }
}

static void CPU_Opcode_JM(void) { cpu.i.value = CPU_FetchShort(); }

static void CPU_Opcode_CA(void) {
    unsigned short address = CPU_FetchShort();
    CPU_Util_CA(address);
}

static void CPU_Opcode_RT(void) { cpu.i.value = CPU_PopShort(); }

static void CPU_Opcode_SIA(void) { CPU_Util_CA(0x0000); }
static void CPU_Opcode_SIB(void) { CPU_Util_CA(0x0008); }
static void CPU_Opcode_SIC(void) { CPU_Util_CA(0x0010); }
static void CPU_Opcode_SID(void) { CPU_Util_CA(0x0018); }
static void CPU_Opcode_SIE(void) { CPU_Util_CA(0x0020); }
static void CPU_Opcode_SIF(void) { CPU_Util_CA(0x0028); }
static void CPU_Opcode_SIG(void) { CPU_Util_CA(0x0030); }
static void CPU_Opcode_SIH(void) { CPU_Util_CA(0x0038); }

/*
    Conditional branching instructions
*/

static void CPU_Opcode_JMZ(void) { CPU_Util_CondJM(cpu.f.z); }
static void CPU_Opcode_JMC(void) { CPU_Util_CondJM(cpu.f.c); }
static void CPU_Opcode_JMS(void) { CPU_Util_CondJM(cpu.f.s); }
static void CPU_Opcode_JMV(void) { CPU_Util_CondJM(cpu.f.v); }

static void CPU_Opcode_JMNZ(void) { CPU_Util_CondJM(!cpu.f.z); }
static void CPU_Opcode_JMNC(void) { CPU_Util_CondJM(!cpu.f.c); }
static void CPU_Opcode_JMNS(void) { CPU_Util_CondJM(!cpu.f.s); }
static void CPU_Opcode_JMNV(void) { CPU_Util_CondJM(!cpu.f.v); }

static void CPU_Opcode_CAZ(void) { CPU_Util_CondCA(cpu.f.z); }
static void CPU_Opcode_CAC(void) { CPU_Util_CondCA(cpu.f.c); }
static void CPU_Opcode_CAS(void) { CPU_Util_CondCA(cpu.f.s); }
static void CPU_Opcode_CAV(void) { CPU_Util_CondCA(cpu.f.v); }

static void CPU_Opcode_CANZ(void) { CPU_Util_CondCA(!cpu.f.z); }
static void CPU_Opcode_CANC(void) { CPU_Util_CondCA(!cpu.f.c); }
static void CPU_Opcode_CANS(void) { CPU_Util_CondCA(!cpu.f.s); }
static void CPU_Opcode_CANV(void) { CPU_Util_CondCA(!cpu.f.v); }

static void CPU_Opcode_RTZ(void) { if (cpu.f.z) cpu.i.value = CPU_PopShort(); }
static void CPU_Opcode_RTC(void) { if (cpu.f.c) cpu.i.value = CPU_PopShort(); }
static void CPU_Opcode_RTS(void) { if (cpu.f.s) cpu.i.value = CPU_PopShort(); }
static void CPU_Opcode_RTV(void) { if (cpu.f.v) cpu.i.value = CPU_PopShort(); }

static void CPU_Opcode_RTNZ(void) { if (!cpu.f.z) cpu.i.value = CPU_PopShort(); }
static void CPU_Opcode_RTNC(void) { if (!cpu.f.c) cpu.i.value = CPU_PopShort(); }
static void CPU_Opcode_RTNS(void) { if (!cpu.f.s) cpu.i.value = CPU_PopShort(); }
static void CPU_Opcode_RTNV(void) { if (!cpu.f.v) cpu.i.value = CPU_PopShort(); }

/*
    I/O port instructions
*/

static void CPU_Opcode_IPB(void) { CPU_PushByte(IO_Read(CPU_FetchByte())); }
static void CPU_Opcode_OPB(void) { IO_Write(CPU_FetchByte(), CPU_PopByte()); }

static void CPU_Opcode_IPS(void) {
    unsigned char port = CPU_FetchByte();

    CPU_PushByte(IO_Read(port + 1));
    CPU_PushByte(IO_Read(port));
}

static void CPU_Opcode_OPS(void) {
    unsigned char port = CPU_FetchByte();

    IO_Write(port, CPU_PopByte());
    IO_Write(port + 1, CPU_PopByte());
}

/*
    Miscellaneous instructions
*/

static void CPU_Opcode_NO(void) { return; }

/*
    CPU opcode function pointer array
*/

static void (*CPU_Opcode[256])(void) = {
    CPU_Opcode_LDAI,
    CPU_Opcode_LDBI,
    CPU_Opcode_LDSI,
    CPU_Opcode_LDAD,
    CPU_Opcode_LDBD,
    CPU_Opcode_LDSD,
    CPU_Opcode_LDARA,
    CPU_Opcode_LDBRA,
    CPU_Opcode_LDSRA,
    CPU_Opcode_LDARB,
    CPU_Opcode_LDBRB,
    CPU_Opcode_LDSRB,
    CPU_Opcode_LDAXA,
    CPU_Opcode_LDBXA,
    CPU_Opcode_LDSXA,
    CPU_Opcode_LDAXB,
    CPU_Opcode_LDBXB,
    CPU_Opcode_LDSXB,
    CPU_Opcode_LDAYA,
    CPU_Opcode_LDBYA,
    CPU_Opcode_LDSYA,
    CPU_Opcode_LDAYB,
    CPU_Opcode_LDBYB,
    CPU_Opcode_LDSYB,
    CPU_Opcode_STAD,
    CPU_Opcode_STBD,
    CPU_Opcode_STSD,
    CPU_Opcode_STARA,
    CPU_Opcode_STBRA,
    CPU_Opcode_STSRA,
    CPU_Opcode_STARB,
    CPU_Opcode_STBRB,
    CPU_Opcode_STSRB,
    CPU_Opcode_STAXA,
    CPU_Opcode_STBXA,
    CPU_Opcode_STSXA,
    CPU_Opcode_STAXB,
    CPU_Opcode_STBXB,
    CPU_Opcode_STSXB,
    CPU_Opcode_STAYA,
    CPU_Opcode_STBYA,
    CPU_Opcode_STSYA,
    CPU_Opcode_STAYB,
    CPU_Opcode_STBYB,
    CPU_Opcode_STSYB,
    CPU_Opcode_MVAB,
    CPU_Opcode_MVAS,
    CPU_Opcode_MVAI,
    CPU_Opcode_MVBA,
    CPU_Opcode_MVBS,
    CPU_Opcode_MVBI,
    CPU_Opcode_MVSA,
    CPU_Opcode_MVSB,
    CPU_Opcode_MVSI,
    CPU_Opcode_MVIA,
    CPU_Opcode_MVIB,
    CPU_Opcode_MVIS,
    CPU_Opcode_PUBI,
    CPU_Opcode_PUBD,
    CPU_Opcode_PUBRA,
    CPU_Opcode_PUBRB,
    CPU_Opcode_PUBXA,
    CPU_Opcode_PUBXB,
    CPU_Opcode_PUBYA,
    CPU_Opcode_PUBYB,
    CPU_Opcode_PUSI,
    CPU_Opcode_PUSD,
    CPU_Opcode_PUSRA,
    CPU_Opcode_PUSRB,
    CPU_Opcode_PUSXA,
    CPU_Opcode_PUSXB,
    CPU_Opcode_PUSYA,
    CPU_Opcode_PUSYB,
    CPU_Opcode_PUA,
    CPU_Opcode_PUB,
    CPU_Opcode_PUS,
    CPU_Opcode_PUI,
    CPU_Opcode_PUF,
    CPU_Opcode_POBD,
    CPU_Opcode_POBRA,
    CPU_Opcode_POBRB,
    CPU_Opcode_POBXA,
    CPU_Opcode_POBXB,
    CPU_Opcode_POBYA,
    CPU_Opcode_POBYB,
    CPU_Opcode_POSD,
    CPU_Opcode_POSRA,
    CPU_Opcode_POSRB,
    CPU_Opcode_POSXA,
    CPU_Opcode_POSXB,
    CPU_Opcode_POSYA,
    CPU_Opcode_POSYB,
    CPU_Opcode_POA,
    CPU_Opcode_POB,
    CPU_Opcode_POS,
    CPU_Opcode_POI,
    CPU_Opcode_POF,
    CPU_Opcode_DTS,
    CPU_Opcode_STS,
    CPU_Opcode_IRA,
    CPU_Opcode_IRB,
    CPU_Opcode_IRS,
    CPU_Opcode_DRA,
    CPU_Opcode_DRB,
    CPU_Opcode_DRS,
    CPU_Opcode_ADB,
    CPU_Opcode_SUB,
    CPU_Opcode_ANB,
    CPU_Opcode_ORB,
    CPU_Opcode_XRB,
    CPU_Opcode_CPB,
    CPU_Opcode_IVB,
    CPU_Opcode_ICB,
    CPU_Opcode_DCB,
    CPU_Opcode_RLB,
    CPU_Opcode_RRB,
    CPU_Opcode_SLB,
    CPU_Opcode_SRB,
    CPU_Opcode_SAB,
    CPU_Opcode_ADS,
    CPU_Opcode_SUS,
    CPU_Opcode_ANS,
    CPU_Opcode_ORS,
    CPU_Opcode_XRS,
    CPU_Opcode_CPS,
    CPU_Opcode_IVS,
    CPU_Opcode_ICS,
    CPU_Opcode_DCS,
    CPU_Opcode_RLS,
    CPU_Opcode_RRS,
    CPU_Opcode_SLS,
    CPU_Opcode_SRS,
    CPU_Opcode_SAS,
    CPU_Opcode_SFZ,
    CPU_Opcode_SFC,
    CPU_Opcode_SFS,
    CPU_Opcode_SFV,
    CPU_Opcode_CFZ,
    CPU_Opcode_CFC,
    CPU_Opcode_CFS,
    CPU_Opcode_CFV,
    CPU_Opcode_EI,
    CPU_Opcode_DI,
    CPU_Opcode_HT,
    CPU_Opcode_JM,
    CPU_Opcode_CA,
    CPU_Opcode_RT,
    CPU_Opcode_SIA,
    CPU_Opcode_SIB,
    CPU_Opcode_SIC,
    CPU_Opcode_SID,
    CPU_Opcode_SIE,
    CPU_Opcode_SIF,
    CPU_Opcode_SIG,
    CPU_Opcode_SIH,
    CPU_Opcode_JMZ,
    CPU_Opcode_JMC,
    CPU_Opcode_JMS,
    CPU_Opcode_JMV,
    CPU_Opcode_JMNZ,
    CPU_Opcode_JMNC,
    CPU_Opcode_JMNS,
    CPU_Opcode_JMNV,
    CPU_Opcode_CAZ,
    CPU_Opcode_CAC,
    CPU_Opcode_CAS,
    CPU_Opcode_CAV,
    CPU_Opcode_CANZ,
    CPU_Opcode_CANC,
    CPU_Opcode_CANS,
    CPU_Opcode_CANV,
    CPU_Opcode_RTZ,
    CPU_Opcode_RTC,
    CPU_Opcode_RTS,
    CPU_Opcode_RTV,
    CPU_Opcode_RTNZ,
    CPU_Opcode_RTNC,
    CPU_Opcode_RTNS,
    CPU_Opcode_RTNV,
    CPU_Opcode_IPB,
    CPU_Opcode_OPB,
    CPU_Opcode_IPS,
    CPU_Opcode_OPS,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO,
    CPU_Opcode_NO
};

int CPU_Init(void) {
    // Reset registers
    cpu.a.value = 0;
    cpu.b.value = 0;
    cpu.s.value = 0;
    cpu.i.value = 0;
    cpu.f.value = 0;

    return 1;
}

void CPU_Execute(void) {
    CPU_Opcode[CPU_FetchByte()]();
}