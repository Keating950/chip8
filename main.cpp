#include <iostream>
#include <stack>
#include <SDL.h>
#include <array>

#include "components.h"

chip8 hardware;
const unsigned short SCREEN_WIDTH = 640u;
const unsigned short SCREEN_HEIGHT = 320u;

void error_exit()
{
    std::cerr << "Error: ";
    std::cerr << SDL_GetError() << std::endl;
    SDL_Quit();
    exit(1);
}

std::pair<SDL_Window *, SDL_Surface *> sdl_setup()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) error_exit();
    SDL_Window *window = SDL_CreateWindow("Chip8 Emulator",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH,
                                          SCREEN_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    // The surface contained by the window
    SDL_Surface *screen_surface = SDL_GetWindowSurface(window);
    if (!(window && screen_surface)) error_exit();
    return {window, screen_surface};
}

SDL_Surface *load_bmp(const char *path)
{
    auto surface = SDL_LoadBMP(path);
    if (!surface) error_exit();
    return surface;
}

// argc, argv format required by SDL
int main(int argc, char **argv)
{
    auto[window, screen_surface] = sdl_setup();
    //Fill the surface white
    SDL_FillRect(screen_surface,
                 nullptr,
                 SDL_MapRGB(screen_surface->format, 0xFF, 0xFF, 0xFF)
    );

    //Update the surface
    SDL_UpdateWindowSurface(window);
    // pump the message/event queue
    SDL_Event event;
    while (SDL_PollEvent(&event));
    //Wait two seconds
    SDL_Delay(2000);
    SDL_DestroyWindow(window);
    //Quit SDL subsystems
    SDL_Quit();
    return 0;
}
