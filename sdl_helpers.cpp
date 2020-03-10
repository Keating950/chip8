#include <iostream>
#include <SDL.h>
#include "sdl_helpers.h"
#include "SDL_image.h"

void sdl_error_exit()
{
    std::cerr << "Error: ";
    std::cerr << SDL_GetError() << std::endl;
    SDL_Quit();
    exit(1);
}

SDL_Surface *load_image(const char *path)
{
    auto surface = IMG_Load(path);
    if (!surface) sdl_error_exit();
    return surface;
}

std::pair<SDL_Window *, SDL_Surface *> sdl_setup()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) sdl_error_exit();
    SDL_Window *window = SDL_CreateWindow("Chip8 Emulator",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH,
                                          SCREEN_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    // The surface contained by the window
    SDL_Surface *screen_surface = SDL_GetWindowSurface(window);
    if (!(window && screen_surface)) sdl_error_exit();
    return {window, screen_surface};
}
