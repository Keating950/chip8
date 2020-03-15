#include <stdio.h>
#include <SDL.h>
#include "sdl_helpers.h"
#include "SDL_image.h"

void sdl_error_exit()
{
    printf("Error: ");
    printf("%s\n", SDL_GetError());
    SDL_Quit();
    exit(1);
}

SDL_Surface *load_image(const char *path)
{
	SDL_Surface *surface = IMG_Load(path);
	if (!surface)
		sdl_error_exit();
	return surface;
}

void init_window(SDL_Window **win, SDL_Surface **surf)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		sdl_error_exit();
	*win = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
				SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	// The surface contained by the window
	*surf = SDL_GetWindowSurface(*win);
	if (!(win && surf))
		sdl_error_exit();

	SDL_FillRect(*surf, NULL,
		     SDL_MapRGB((*surf)->format, 0x00, 0x00, 0x00));
}
