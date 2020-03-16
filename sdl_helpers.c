#include <stdio.h>
#include <SDL.h>
#include "sdl_helpers.h"
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define handle_error(msg)                                                      \
	do {                                                                   \
		perror(msg);                                                   \
		SDL_Quit();                                                    \
		exit(EXIT_FAILURE);                                            \
	} while (0)

void init_window(SDL_Window **win, SDL_Surface **surf)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		handle_error("Could not init sdl");
	*win = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
				SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	// The surface contained by the window
	*surf = SDL_GetWindowSurface(*win);
	if (!(win))
		handle_error("Failed to initialize SDL window");
	if (!(surf))
		handle_error("Failed to initialize SDL surface");
	SDL_FillRect(*surf, NULL,
		     SDL_MapRGB((*surf)->format, 0x00, 0x00, 0x00));
}
