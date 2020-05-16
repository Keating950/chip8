#ifdef __APPLE__
#include <sys/cdefs.h>
#endif
#define _BSD_SOURCE
#include <SDL.h>
#include <SDL_video.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "chip8.h"
#include "util.h"

#define TIMER_HZ_NS 16666667
#define FRAME_NS 2000000
#define OPS_PER_FRAME 8

#define SCREEN_WIDTH 0x40
#define SCREEN_HEIGHT 0x20

static void init_window(void);
void destroy_window(void);
int init_audio(void);
int scancode_to_chip8(int scancode);
void draw_screen(const chip8_vm *vm);
void main_loop(chip8_vm *vm);
static inline void difftime_ns(const struct timespec *then,
			   const struct timespec *now,
			   struct timespec *result);

static SDL_Window *win = NULL;

// argc, argv format required by SDL
int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Wrong number of arguments.\n"
						"Usage:\n\tchip8 [path to rom]\n");
		exit(EXIT_FAILURE);
	}
	init_window();
	atexit(destroy_window);
	chip8_vm vm = init_chip8();
	load_rom(argv[1], &vm);
	main_loop(&vm);
	SDL_DestroyWindow(win);
	exit(EXIT_SUCCESS);
}

void main_loop(chip8_vm *vm)
{
	SDL_Event event;
	struct timespec frame_start;
	struct timespec frame_end;
	struct timespec delay = { 0, 0 };
	int ops_since_draw = 0;
	int key;
	int key_pressed = 0;
	int delta;

	while (1) {
		if (!ops_since_draw)
			clock_gettime(CLOCK_MONOTONIC, &frame_start);
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN: // FALLTHROUGH
				key_pressed = 1;
			case SDL_KEYUP:
				key = scancode_to_chip8(event.key.keysym.scancode);
				if (key > 0)
					vm->keyboard[key] = !vm->keyboard[key];
			default:
				continue;
			}
		}
		vm_cycle(vm, key_pressed);
		key_pressed = 0;
		// TODO: add audio

		if (ops_since_draw++ == OPS_PER_FRAME) {
			ops_since_draw = 0;
			vm->delay_timer = ZERO_FLOOR(vm->delay_timer - 1);
			vm->sound_timer = ZERO_FLOOR(vm->sound_timer - 1);
			draw_screen(vm);
			SDL_UpdateWindowSurface(win);
			clock_gettime(CLOCK_MONOTONIC, &frame_end);
			difftime_ns(&frame_start, &frame_end, &delay);
			delta = (TIMER_HZ_NS - delay.tv_nsec) / 1000;
			if (ZERO_FLOOR(delta))
				usleep(delta);
		}
	}
}

void destroy_window(void)
{
	if (win)
		SDL_DestroyWindow(win);
}

static inline void difftime_ns(const struct timespec *then,
							   const struct timespec *now,
							   struct timespec *result)
{
	if ((now->tv_nsec - then->tv_nsec) < 0) {
		result->tv_nsec = now->tv_nsec - then->tv_nsec + 1000000000;
	} else {
		result->tv_nsec = now->tv_nsec - then->tv_nsec;
	}
}

void draw_screen(const chip8_vm *vm)
{
	SDL_Surface *screen = SDL_CreateRGBSurfaceWithFormatFrom(
		(void *)vm->screen, SCREEN_WIDTH, SCREEN_HEIGHT, 24, // bit depth
		SCREEN_WIDTH * 4, SDL_GetWindowPixelFormat(win));
	SDL_BlitSurface(screen, NULL, SDL_GetWindowSurface(win), NULL);
	SDL_FreeSurface(screen);
}

int init_audio(void)
{
	SDL_AudioSpec spec, act_spec;
	SDL_zero(spec);
	spec.freq = 48000;
	spec.format = AUDIO_S16SYS;
	spec.channels = 1;
	spec.samples = 4096;
	int id = SDL_OpenAudioDevice(NULL, 0, &spec, &act_spec,
								 SDL_AUDIO_ALLOW_ANY_CHANGE);
	if (id <= 0)
		ERROR_EXIT(SDL_GetError());
	return id;
}

static void init_window(void)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		ERROR_EXIT("Could not initialize SDL");
	win = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_UNDEFINED,
						   SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT,
						   SDL_WINDOW_SHOWN);
	if (!(win))
		ERROR_EXIT("Failed to create window");
	SDL_Surface *surf = SDL_GetWindowSurface(win);
	if (!(surf))
		ERROR_EXIT("Failed to create SDL surface");
	SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, 0, 0, 0));
	SDL_UpdateWindowSurface(win);
}

int scancode_to_chip8(int scancode)
{
	switch (scancode) {
	// 123C
	case SDL_SCANCODE_X:
		return 0x0;
	case SDL_SCANCODE_1:
		return 0x1;
	case SDL_SCANCODE_2:
		return 0x2;
	case SDL_SCANCODE_3:
		return 0x3;
	case SDL_SCANCODE_Q:
		return 0x4;
	// 456D
	case SDL_SCANCODE_W:
		return 0x5;
	case SDL_SCANCODE_E:
		return 0x6;
	case SDL_SCANCODE_A:
		return 0x7;
	case SDL_SCANCODE_S:
		return 0x8;
	// 789E
	case SDL_SCANCODE_D:
		return 0x9;
	case SDL_SCANCODE_Z:
		return 0xA;
	case SDL_SCANCODE_C:
		return 0xB;
	// A0BF
	case SDL_SCANCODE_4:
		return 0xC;
	case SDL_SCANCODE_R:
		return 0xD;
	case SDL_SCANCODE_F:
		return 0xE;
	case SDL_SCANCODE_V:
		return 0xF;
	default:
		return -1;
	}
}
