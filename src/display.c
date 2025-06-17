#include "display.h"

#include <SDL3/SDL.h>
#include <stdio.h>

#include "io.h"
#include "font.h"
#include "memory.h"

// The window
static SDL_Window *WINDOW = NULL;

// The window surface
static SDL_Surface *SURFACE = NULL;

// Display mode macros
#define DISPLAY_MODE_COUNT 8
#define DISPLAY_MODE_COUNT_MASK 7

// Display memory size array
static unsigned short DISPLAY_MEMORY_SIZE[DISPLAY_MODE_COUNT] = {
    1000,   // 40 x 25, monochrome
    2000,   // 40 x 25, 16 colors
    4000,   // 80 x 50, monochrome
    8000,   // 80 x 50, 16 colors
    8000,   // 320 x 200, monochrome
    32000,  // 320 x 200, 16 colors
    8000,   // same as mode 4
    32000,  // same as mode 5
};

// Display width / column count array
static unsigned short DISPLAY_WIDTH[DISPLAY_MODE_COUNT] = {
    40,
    40,
    80,
    80,
    320,
    320,
    320,
    320,
};

// Display height / row count array
static unsigned short DISPLAY_HEIGHT[DISPLAY_MODE_COUNT] = {
    25,
    25,
    50,
    50,
    200,
    200,
    200,
    200,
};

// Color palette array
static unsigned DISPLAY_PALETTE[16] = {
    0x00000000,
    0x00000080,
    0x00008000,
    0x00008080,
    0x00800000,
    0x00800080,
    0x00808000,
    0x00808080,
    
    0x00C0C0C0,
    0x000000FF,
    0x0000FF00,
    0x0000FFFF,
    0x00FF0000,
    0x00FF00FF,
    0x00FFFF00,
    0x00FFFFFF,
};

// Display struct
static struct {
    // Starting address of video memory
    unsigned short base;

    DisplayMode mode;

    // Cursor position for text mode
    unsigned char cursorX, cursorY;

    // Cursor type for text mode
    union {
        struct {
            unsigned char cursorEnable:1;
            unsigned char cursorBlink:1;
        };

        unsigned char value;
    } cursorType;

    // Data registers
    union {
        struct {
            unsigned char lo;
            unsigned char hi;
        };

        unsigned value;
    } data, cursorIndex;
} display;

// Function to set a pixel on the display
static void Display_SetPixel(unsigned x, unsigned y, unsigned pixel) {
    x = SDL_min(x, WINDOW_W - 1);
    y = SDL_min(y, WINDOW_H - 1);

    unsigned *pixels = (unsigned*) SURFACE->pixels + y * SURFACE->w + x;

    *pixels = pixel;
}

// Function to set a double - sized pixel on the display
static void Display_SetPixelDouble(unsigned x, unsigned y, unsigned pixel) {
    x = SDL_min(x, WINDOW_HW - 1) << 1;
    y = SDL_min(y, WINDOW_HH - 1) << 1;

    unsigned *pixels = (unsigned*) SURFACE->pixels + y * SURFACE->w + x;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            *(pixels + j) = pixel;
        }

        pixels += SURFACE->w;
    }
}

// Command port write function
static void Display_CommandPortWrite(unsigned char byte) {
    switch (byte) {
        case DISPLAY_COMMAND_GET_MEMORY_SIZE:
            display.data.value = DISPLAY_MEMORY_SIZE[display.mode];
            break;

        case DISPLAY_COMMAND_GET_WIDTH:
            display.data.value = DISPLAY_WIDTH[display.mode];
            break;

        case DISPLAY_COMMAND_GET_HEIGHT:
            display.data.value = DISPLAY_HEIGHT[display.mode];
            break;


        case DISPLAY_COMMAND_GET_MODE:
            display.data.lo = display.mode;
            display.data.hi = display.mode;

            break;

        case DISPLAY_COMMAND_GET_CURSOR_INDEX:
            display.data.value = display.cursorIndex.value;
            break;

        case DISPLAY_COMMAND_GET_CURSOR_X:
            display.data.lo = display.cursorX;
            display.data.hi = display.cursorX;

            break;

        case DISPLAY_COMMAND_GET_CURSOR_Y:
            display.data.lo = display.cursorY;
            display.data.hi = display.cursorY;

            break;

        case DISPLAY_COMMAND_GET_CURSOR_TYPE:
            display.data.lo = display.cursorType.value;
            display.data.hi = display.cursorType.value;

            break;


        case DISPLAY_COMMAND_SET_MODE:
            display.mode = display.data.lo & DISPLAY_MODE_COUNT_MASK;
            break;

        case DISPLAY_COMMAND_SET_CURSOR_INDEX:
            // Set cursor x
            display.cursorX = display.data.value % DISPLAY_WIDTH[display.mode];

            // Set cursor y
            display.cursorY = display.data.value / DISPLAY_WIDTH[display.mode];
            display.cursorY %= DISPLAY_HEIGHT[display.mode];

            // Set index
            display.cursorIndex.value = display.cursorY * DISPLAY_WIDTH[display.mode] + display.cursorX;

            break;

        case DISPLAY_COMMAND_SET_CURSOR_X:
            display.cursorX = display.data.lo % DISPLAY_WIDTH[display.mode];
            display.cursorIndex.value = display.cursorY * DISPLAY_WIDTH[display.mode] + display.cursorX;

            break;

        case DISPLAY_COMMAND_SET_CURSOR_Y:
            display.cursorY = display.data.lo % DISPLAY_HEIGHT[display.mode];
            display.cursorIndex.value = display.cursorY * DISPLAY_WIDTH[display.mode] + display.cursorX;

            break;
        
        case DISPLAY_COMMAND_SET_CURSOR_POS:
            display.cursorX = display.data.lo % DISPLAY_WIDTH[display.mode];
            display.cursorY = display.data.hi % DISPLAY_HEIGHT[display.mode];

            display.cursorIndex.value = display.cursorY * DISPLAY_WIDTH[display.mode] + display.cursorX;

            break;

        case DISPLAY_COMMAND_SET_CURSOR_TYPE:
            display.cursorType.value = display.data.lo;
            break;

        default:
            break;
    }
}

// Data port functions
static unsigned char Display_DataLoRead(void) { return display.data.lo; }
static unsigned char Display_DataHiRead(void) { return display.data.hi; }

static void Display_DataLoWrite(unsigned char byte) { display.data.lo = byte; }
static void Display_DataHiWrite(unsigned char byte) { display.data.hi = byte; }

// Function to draw a character
static void Display_DrawChar(unsigned x, unsigned y, unsigned char c, unsigned char color) {
    c = SDL_clamp(c - ' ', 0, FONT_CHAR_COUNT - 1);

    unsigned long long charData = FONT_DATA[c];

    unsigned colors[2] = {
        DISPLAY_PALETTE[color >> 4],
        DISPLAY_PALETTE[color & 0x0F],
    };

    for (unsigned char i = 0; i < 8; i++)
        for (unsigned char j = 0; j < 8; j++) {
            Display_SetPixel(x + j, y + i, colors[charData & 1]);

            charData >>= 1;
        }
}

// Function to draw a character (double size)
static void Display_DrawCharDouble(unsigned x, unsigned y, unsigned char c, unsigned char color) {
    c = SDL_clamp(c - ' ', 0, FONT_CHAR_COUNT - 1);

    unsigned long long charData = FONT_DATA[c];

    unsigned colors[2] = {
        DISPLAY_PALETTE[color >> 4],
        DISPLAY_PALETTE[color & 0x0F],
    };

    for (unsigned char i = 0; i < 8; i++)
        for (unsigned char j = 0; j < 8; j++) {
            Display_SetPixelDouble(x + j, y + i, colors[charData & 1]);

            charData >>= 1;
        }
}

// Function to draw display in text mode, normal sized, monochrome
static void Display_DrawTextMono(void) {
    unsigned short address = display.base;

    unsigned drawX, drawY = 0;

    for (int i = 0; i < DISPLAY_HEIGHT[display.mode]; i++) {
        drawX = 0;

        for (int j = 0; j < DISPLAY_WIDTH[display.mode]; j++) {
            Display_DrawChar(drawX, drawY, Memory_GetByte(address++), 0x08);

            drawX += FONT_CHAR_SIZE;
        }

        drawY += FONT_CHAR_SIZE;
    }
}

// Function to draw display in text mode, double sized, monochrome
static void Display_DrawTextDoubleMono(void) {
    unsigned short address = display.base;

    unsigned drawX, drawY = 0;

    for (int i = 0; i < DISPLAY_HEIGHT[display.mode]; i++) {
        drawX = 0;

        for (int j = 0; j < DISPLAY_WIDTH[display.mode]; j++) {
            Display_DrawCharDouble(drawX, drawY, Memory_GetByte(address++), 0x08);

            drawX += FONT_CHAR_SIZE;
        }

        drawY += FONT_CHAR_SIZE;
    }
}

// Function to draw display in text mode, normal sized, 16 colors
static void Display_DrawText16(void) {
    unsigned short address = display.base;

    unsigned drawX, drawY = 0;

    for (int i = 0; i < DISPLAY_HEIGHT[display.mode]; i++) {
        drawX = 0;

        for (int j = 0; j < DISPLAY_WIDTH[display.mode]; j++) {
            unsigned char c = Memory_GetByte(address++);
            unsigned char color = Memory_GetByte(address++);

            Display_DrawChar(drawX, drawY, c, color);

            drawX += FONT_CHAR_SIZE;
        }

        drawY += FONT_CHAR_SIZE;
    }
}

// Function to draw display in text mode, double sized, 16 colors
static void Display_DrawTextDouble16(void) {
    unsigned short address = display.base;

    unsigned drawX, drawY = 0;

    for (int i = 0; i < DISPLAY_HEIGHT[display.mode]; i++) {
        drawX = 0;

        for (int j = 0; j < DISPLAY_WIDTH[display.mode]; j++) {
            unsigned char c = Memory_GetByte(address++);
            unsigned char color = Memory_GetByte(address++);

            Display_DrawCharDouble(drawX, drawY, c, color);

            drawX += FONT_CHAR_SIZE;
        }

        drawY += FONT_CHAR_SIZE;
    }
}

// Function to draw display in pixel mode, monochrome
static void Display_DrawPixelMono(void) {
    unsigned char pixel;
    unsigned char shift;
    unsigned pixelIndex = 0;
    unsigned address;

    for (int i = 0; i < DISPLAY_HEIGHT[display.mode]; i++) {
        for (int j = 0; j < DISPLAY_WIDTH[display.mode]; j++) {
            address = (pixelIndex >> FONT_CHAR_SIZE_SHIFT) + display.base;
            shift = pixelIndex++ & FONT_CHAR_SIZE_MASK;
            pixel = (Memory_GetByte(address) >> shift) & 1;

            Display_SetPixelDouble(j, i, 0xABCDEF & -pixel);
        }
    }
}

// Function to draw display in pixel mode, 16 colors
static void Display_DrawPixel16(void) {
    unsigned char pixel;
    unsigned char shift;
    unsigned pixelIndex = 0;
    unsigned address;

    for (int i = 0; i < DISPLAY_HEIGHT[display.mode]; i++) {
        for (int j = 0; j < DISPLAY_WIDTH[display.mode]; j++) {
            address = (pixelIndex >> 1) + display.base;
            shift = 4 & -(pixelIndex++ & 1);
            pixel = (Memory_GetByte(address) >> shift) & 15;

            Display_SetPixelDouble(j, i, DISPLAY_PALETTE[pixel]);
        }
    }
}

// Display drawing functions array
static void (*Display_DrawFunction[DISPLAY_MODE_COUNT])(void) = {
    Display_DrawTextDoubleMono,
    Display_DrawTextDouble16,
    Display_DrawTextMono,
    Display_DrawText16,
    Display_DrawPixelMono,
    Display_DrawPixel16,
    Display_DrawPixelMono,
    Display_DrawPixel16,
};

int Display_Init(void) {
    WINDOW = SDL_CreateWindow(
        "Stinky Stacky Virtual Machine",
        WINDOW_W, WINDOW_H,
        0
    );

    if (WINDOW == NULL) {
        printf("Error creating window: %s\n", SDL_GetError());

        return 0;
    }

    SURFACE = SDL_GetWindowSurface(WINDOW);
    
    // Register IO commands
    IO_RegisterWrite(DISPLAY_PORT_COMMAND, Display_CommandPortWrite);

    IO_RegisterRead(DISPLAY_PORT_DATA_LO, Display_DataLoRead);
    IO_RegisterRead(DISPLAY_PORT_DATA_HI, Display_DataHiRead);

    IO_RegisterWrite(DISPLAY_PORT_DATA_LO, Display_DataLoWrite);
    IO_RegisterWrite(DISPLAY_PORT_DATA_HI, Display_DataHiWrite);

    // Set initial display mode to 40 x 20 monochrome text mode
    display.mode = DISPLAY_MODE_TEXT_40_30_2;

    return 1;
}

void Display_Draw(void) {
    SDL_LockSurface(SURFACE);

    Display_DrawFunction[display.mode]();

    SDL_UnlockSurface(SURFACE);
    SDL_UpdateWindowSurface(WINDOW);

    SDL_Delay(16);
}

void Display_Quit(void) {
    SDL_DestroyWindow(WINDOW);
}