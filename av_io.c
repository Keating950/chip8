#include "av_io.h"
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define ERROR_EXIT(msg)                                                   \
	do {                                                              \
		perror(msg);                                              \
		exit(EXIT_FAILURE);                                       \
	} while (0)

SDL_Window *init_window()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		ERROR_EXIT("Could not init sdl");
	SDL_Window *win =
		SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_UNDEFINED,
				 SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
				 SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	SDL_Surface *surf = SDL_GetWindowSurface(win);
	SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, 0, 0, 0));
	SDL_UpdateWindowSurface(win);
	return win;
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

/*
void print_vm_screen(const chip8_vm *vm)
{
	for (int i=0; i<0x20; i++) {
		for (int j=0; j<0x40; j++) {
			int px = vm->screen[i][j] > 0 ? 1 : 0;
			printf("%2i", px);
		}
		puts("\n");
	}
}
*/

void draw_screen(const chip8_vm *vm, SDL_Window *win)
{
	SDL_Surface *screen = SDL_CreateRGBSurfaceWithFormatFrom(
		(void *) vm->screen, 0x40, 0x20, 24, 0x40 * 4,
		SDL_GetWindowPixelFormat(win));
	SDL_BlitSurface(screen, NULL, SDL_GetWindowSurface(win), NULL);
	return;
}
