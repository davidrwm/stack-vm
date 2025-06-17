#ifndef __UTILS_H__
#define __UTILS_H__

// Macro to get the low byte of a short
#define SHORT_LO(x) ((x) & 0xFF)

// Macro to get the high byte of a short
#define SHORT_HI(x) SHORT_LO((x) >> 8)

// Macro to combine two bytes into a short
#define TO_SHORT(lo, hi) (((hi) << 8) | (lo))

#endif