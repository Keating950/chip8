#define _GNU_SOURCE
#include "../lib/chip8.h"
#include "../lib/av_io.h"
#include <SDL.h>
#include <SDL_video.h>
#include <stdio.h>
#include <time.h>
#ifdef __MACH__ // macOS compatibility
timing_mach_init();
#endif
#define error_crash(msg)                                                       \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)
#define zero_floor(n) n > 0 ? n : 0
#define HZ_NS_CONV_FACTOR 6e-8

short scancode_to_chip8(int scancode)
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

void main_loop(chip8_vm *vm, SDL_Window *win, SDL_Surface *surf, int audio_id)
{
	SDL_Event event;
	short key;
	struct timespec lastread_ts;
	struct timespec current_ts;
	long time_delta;

	clock_gettime(CLOCK_MONOTONIC, &lastread_ts);
	while (true) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				return;
			else if (event.type == SDL_KEYDOWN
				 || event.type == SDL_KEYUP) {
				key = scancode_to_chip8(event.key.keysym.sym);
				if (key > -1)
					vm->keyboard[key]
						= !(vm->keyboard[key]);
			} else {
				continue;
			}
		}

		clock_gettime(CLOCK_MONOTONIC, &current_ts);
		time_delta = (current_ts.tv_nsec - lastread_ts.tv_nsec)
			     * HZ_NS_CONV_FACTOR;
		vm->delay_timer = zero_floor(vm->delay_timer - time_delta);
		vm->sound_timer = zero_floor(vm->delay_timer - time_delta);
		clock_gettime(CLOCK_MONOTONIC, &lastread_ts);

		vm_cycle(vm);

		if (SDL_GetAudioDeviceStatus(audio_id) == SDL_AUDIO_PLAYING
		    && vm->sound_timer <= 0x00) {
			SDL_PauseAudioDevice(audio_id, 1);
		}
		if (SDL_GetAudioDeviceStatus(audio_id) == SDL_AUDIO_PAUSED
		    && vm->sound_timer >= 0x02) {
			SDL_PauseAudioDevice(audio_id, 0);
		}
		if (vm->draw_flag) {
			draw_screen(vm, &surf);
			SDL_UpdateWindowSurface(win);
		}
	}
}

// argc, argv format required by SDL
int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Wrong number of arguments. "
				"Usage:\tchip8 [path to rom]");
		exit(1);
	}
	SDL_Window *win;
	SDL_Surface *surf;
	init_window(&win, &surf);
	int audio_id = init_audio();
	chip8_vm vm = init_chip8();
	load_rom(argv[1], &vm);
	main_loop(&vm, win, surf, audio_id);
	SDL_DestroyWindow(win);
	SDL_Quit();
	exit(EXIT_SUCCESS);
}
