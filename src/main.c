#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

#include "cpu.h"
#include "display.h"
#include "disk.h"
#include "memory.h"
#include "io.h"

SDL_AppResult SDL_AppInit(void **appState, int argc, char **argv) {
    // Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) return SDL_APP_FAILURE;

    // Initialize the CPU
    printf("Initializing CPU...\n");
    if (!CPU_Init()) return SDL_APP_FAILURE;

    // Initialize the memory
    if (!Memory_Init()) return SDL_APP_FAILURE;

    // Initialize I/O module
    printf("Initializing I/O module...\n");
    if (!IO_Init()) return SDL_APP_FAILURE;

    // Initialize the disk
    printf("Initializing disk...\n");
    if (!Disk_Init()) return SDL_APP_FAILURE;

    // Initialize the display
    printf("Initializing display...\n");
    if (!Display_Init()) return SDL_APP_FAILURE;
    
    printf("Entering main loop...\n");

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appState, SDL_Event *event) {
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        
        case SDL_EVENT_KEY_DOWN:
            break;
    
        case SDL_EVENT_KEY_UP:
            switch (event->key.key) {
                case SDLK_ESCAPE:
                    return SDL_APP_SUCCESS;
    
                default:
                    break;
            }
    
            break;
    
        default:
            break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appState) {
    // Update modules
    Disk_Update();

    Display_Draw();

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appState, SDL_AppResult result) {
    printf("Quitting all modules...\n");

    if (result == SDL_APP_FAILURE)
        printf("Error: %s\n", SDL_GetError());
    else
        printf("Quit successfully!\n");

    Display_Quit();
    Disk_Quit();
    SDL_Quit();
}