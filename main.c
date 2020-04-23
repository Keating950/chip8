#include "chip8.h"
#include <SDL.h>
#include <SDL_video.h>
#include <stdio.h>
#define ERROR_EXIT(msg)                                                   \
	do {                                                                  \
		perror(msg);                                                      \
		exit(EXIT_FAILURE);                                               \
	} while (0)
#define ZERO_FLOOR(N) ((N) > 0 ? (N) : 0)
#define SCREEN_WIDTH 0x40
#define SCREEN_HEIGHT 0x20
#define FRAME_DURATION 16f

SDL_Window *init_window(void)
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

int init_audio(void)
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
		ERROR_EXIT(SDL_GetError());
	}
	return id;
}

void draw_screen(const chip8_vm *vm, SDL_Window *win)
{
	SDL_Surface *screen =
		SDL_CreateRGBSurfaceWithFormatFrom((void *)vm->screen, 0x40, 0x20,
										   24, 0x40 * 4,
										   SDL_GetWindowPixelFormat(win));
	SDL_BlitSurface(screen, NULL, SDL_GetWindowSurface(win), NULL);
	return;
}

int scancode_to_chip8(int scancode)
{
	switch (scancode) {
	// 123C
	case SDL_SCANCODE_6:
		return 0x1;
	case SDL_SCANCODE_7:
		return 0x2;
	case SDL_SCANCODE_8:
		return 0x3;
	case SDL_SCANCODE_9:
		return 0xC;
	// 456D
	case SDL_SCANCODE_Y:
		return 0x4;
	case SDL_SCANCODE_U:
		return 0x5;
	case SDL_SCANCODE_I:
		return 0x6;
	case SDL_SCANCODE_O:
		return 0xD;
	// 789E
	case SDL_SCANCODE_H:
		return 0x7;
	case SDL_SCANCODE_J:
		return 0x8;
	case SDL_SCANCODE_K:
		return 0x9;
	case SDL_SCANCODE_L:
		return 0xE;
	// A0BF
	case SDL_SCANCODE_N:
		return 0xA;
	case SDL_SCANCODE_M:
		return 0x0;
	case SDL_SCANCODE_COMMA:
		return 0xB;
	case SDL_SCANCODE_PERIOD:
		return 0xF;
	default:
		return -1;
	}
}

uint8_t await_keypress(void)
{
	SDL_Event event;
	int keypress;
	do {
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT)
			exit(EXIT_SUCCESS);
		else if (event.type == SDL_KEYDOWN) {
			keypress = scancode_to_chip8(event.key.keysym.sym);
			if (keypress > 0)
				return (uint8_t)keypress;
		}
	} while (1);
}

void main_loop(chip8_vm *vm, SDL_Window *win)
{
	SDL_Event event;
	int key;
	unsigned cycle_start;
	while (true) {
		cycle_start = SDL_GetTicks();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				key = scancode_to_chip8(event.key.keysym.sym);
				if (key > -1)
					vm->keyboard[key] = !(vm->keyboard[key]);
			default:
				continue;
			}
		}
		vm_cycle(vm);
		// TODO: add audio
		if (vm->draw_flag) {
			draw_screen(vm, win);
			SDL_UpdateWindowSurface(win);
			vm->draw_flag = false;
		}
		vm->delay_timer--;
		vm->sound_timer--;
		SDL_Delay(SDL_GetTicks() - cycle_start);
	}
}

// argc, argv format required by SDL
int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Wrong number of arguments.\n"
						"Usage:\n\tchip8 [path to rom]\n");
		exit(EXIT_FAILURE);
	}
	SDL_Window *win = init_window();
	chip8_vm vm = init_chip8();
	load_rom(argv[1], &vm);
	main_loop(&vm, win);
	free(win);
	exit(EXIT_SUCCESS);
}
