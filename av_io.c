//
// Created by bigfootlives on 2020-03-19.
//

#include "av_io.h"
//#include "SDL_image.h"
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define ERROR_EXIT(msg)                                                        \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)

void init_window(SDL_Window **win)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		ERROR_EXIT("Could not init sdl");
	*win = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
				SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	SDL_Surface *surf = SDL_GetWindowSurface(*win);
	SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, 0, 0, 0));
	SDL_UpdateWindowSurface(*win);
}

int init_audio()
{
	SDL_AudioSpec spec, act_spec; // the specs of our piece of "music"
	SDL_zero(spec);
	spec.freq = 48000;
	spec.format = AUDIO_S16SYS;
	spec.channels = 1;
	spec.samples = 4096;
	int id;
	if ((id = SDL_OpenAudioDevice(NULL, 0, &spec, &act_spec,
				      SDL_AUDIO_ALLOW_ANY_CHANGE)) <= 0) {
		ERROR_EXIT(SDL_GetError());
	}
	return id;
}

void draw_screen(const chip8_vm *vm, SDL_Window **win)
{
	const uint32_t window_format = SDL_GetWindowPixelFormat(*win);
	SDL_Surface *vmscreen_surf = SDL_CreateRGBSurfaceWithFormatFrom(
		(void *)vm->screen, 0x40, 0x20,
		SDL_BITSPERPIXEL(window_format),
		SCREEN_WIDTH * sizeof(vm->screen[0][0]), window_format);
	if (!vmscreen_surf)
		ERROR_EXIT(SDL_GetError());
	SDL_BlitSurface(vmscreen_surf, NULL, SDL_GetWindowSurface(*win),
		      NULL);
}
