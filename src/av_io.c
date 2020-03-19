//
// Created by bigfootlives on 2020-03-19.
//

#include "../lib/av_io.h"
//#include "SDL_image.h"
#define SCREEN_WIDTH 0x40
#define SCREEN_HEIGHT 0x20
#define error_crash(msg)                                                       \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)

void init_window(SDL_Window **win, SDL_Surface **surf)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		error_crash("Could not init sdl");
	*win = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
				SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

	// The surface contained by the window
	*surf = SDL_GetWindowSurface(*win);
	(*surf)->format = SDL_AllocFormat(SDL_PIXELFORMAT_INDEX1MSB);
	if (!*win)
		error_crash("Failed to initialize SDL window");
	if (!*surf)
		error_crash("Failed to initialize SDL surface");
	SDL_FillRect(*surf, NULL,
		     SDL_MapRGB((*surf)->format, 0x00, 0x00, 0x00));
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
				      SDL_AUDIO_ALLOW_ANY_CHANGE))
	    <= 0) {
		error_crash(SDL_GetError());
	}
	return id;
}

void draw_screen(chip8_vm *vm, SDL_Surface **win_surf)
{
	//	static const SDL_PixelFormat PIXELF
	//		= { SDL_PIXELFORMAT_INDEX1MSB, NULL, 1, 1, 1, 0, 0, 0 };
	//	SDL_Surface *sprite;
	(*win_surf)->pixels = vm->screen;
	return;
}
