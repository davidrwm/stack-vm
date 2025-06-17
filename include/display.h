#ifndef __DISPLAY_H__
#define __DISPLAY_H__

// Window dimension constants
#define WINDOW_W 640
#define WINDOW_H 400

#define WINDOW_HW (WINDOW_W >> 1)
#define WINDOW_HH (WINDOW_H >> 1)

// Display port enum
typedef enum display_port_e {
    DISPLAY_PORT_COMMAND = 0x30,
    DISPLAY_PORT_DATA_LO,
    DISPLAY_PORT_DATA_HI
} DisplayPort;

// Display command enums
typedef enum display_command_e {
    DISPLAY_COMMAND_GET_MEMORY_SIZE = 0x00,
    DISPLAY_COMMAND_GET_WIDTH,
    DISPLAY_COMMAND_GET_HEIGHT,

    DISPLAY_COMMAND_GET_MEMORY_BASE = 0x10,
    DISPLAY_COMMAND_GET_MODE,
    DISPLAY_COMMAND_GET_CURSOR_INDEX,
    DISPLAY_COMMAND_GET_CURSOR_X,
    DISPLAY_COMMAND_GET_CURSOR_Y,
    DISPLAY_COMMAND_GET_CURSOR_POS,
    DISPLAY_COMMAND_GET_CURSOR_TYPE,

    DISPLAY_COMMAND_SET_MEMORY_BASE = 0x20,
    DISPLAY_COMMAND_SET_MODE,
    DISPLAY_COMMAND_SET_CURSOR_INDEX,
    DISPLAY_COMMAND_SET_CURSOR_X,
    DISPLAY_COMMAND_SET_CURSOR_Y,
    DISPLAY_COMMAND_SET_CURSOR_POS,
    DISPLAY_COMMAND_SET_CURSOR_TYPE,
} DisplayCommand;

// Display mode enum
typedef enum display_mode_e {
    DISPLAY_MODE_TEXT_40_30_2 = 0x00,
    DISPLAY_MODE_TEXT_40_30_16,
    DISPLAY_MODE_TEXT_80_60_2,
    DISPLAY_MODE_TEXT_80_60_16,
    DISPLAY_MODE_PIXEL_320_200_2,
    DISPLAY_MODE_PIXEL_320_200_16,
    DISPLAY_MODE_PIXEL_320_200_2_COPY,
    DISPLAY_MODE_PIXEL_320_200_16_COPY,
} DisplayMode;

// Function to initialize the display
int Display_Init(void);

// Function to draw the display
void Display_Draw(void);

// Function to quit the display
void Display_Quit(void);

#endif