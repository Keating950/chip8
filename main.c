#define _GNU_SOURCE

#include "chip8.h"
#include "av_io.h"
#include <SDL.h>
#include <SDL_video.h>
#include <stdio.h>
#include <unistd.h>

#define ERROR_EXIT(msg)                                                   \
	do {                                                              \
		perror(msg);                                              \
		exit(EXIT_FAILURE);                                       \
	} while (0)

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

void main_loop(chip8_vm *vm, SDL_Window *win)
{
	SDL_Event event;
	int key;
	unsigned int cycle_start;
	while (true) {
		cycle_start=SDL_GetTicks();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				key = scancode_to_chip8(
					event.key.keysym.sym);
				if (key > -1)
					vm->keyboard[key] =
						!(vm->keyboard[key]);
			default:
				continue;
			}
		}
		vm_cycle(vm);
		// TODO: add audio
		if (vm->draw_flag) {
			draw_screen(vm, win);
			SDL_UpdateWindowSurface(win);
			vm->draw_flag=false;
		}
		vm->delay_timer--;
		vm->sound_timer--;
//		SDL_Delay(SDL_GetTicks()-cycle_start);
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
