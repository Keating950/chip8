#include <SDL.h>
#include <SDL_video.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "chip8.h"
#include "util.h"

SDL_Window *init_window(void);
void main_loop(chip8_vm *vm, SDL_Window *win);
static void catch_exit(int signo);

jmp_buf env;

// argc, argv format required by SDL
int main(int argc, char **argv)
{
	int jumpval;
	/* if (argc < 2) { */
	/* 	fprintf(stderr, "Wrong number of arguments.\n" */
	/* 					"Usage:\n\tchip8 [path to rom]\n"); */
	/* 	exit(EXIT_FAILURE); */
	/* } */
	signal(SIGINT, catch_exit);
	signal(SIGTERM, catch_exit);
	signal(SIGQUIT, catch_exit);
	signal(SIGHUP, catch_exit);
	SDL_Window *win = init_window();
	chip8_vm vm = init_chip8();
	/* load_rom(argv[1], &vm); */
	load_rom("roms/addition.ch8", &vm);
	jumpval = setjmp(env);
	if (!(jumpval))
		main_loop(&vm, win);
	SDL_DestroyWindow(win);
	exit(EXIT_SUCCESS);
}

static void catch_exit(int signo)
{
	switch (signo) {
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
	case SIGHUP:
		longjmp(env, 1);
	}
}

SDL_Window *init_window(void)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		ERROR_EXIT("Could not initialize SDL");
	SDL_Window *win =
		SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_UNDEFINED,
						 SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
						 SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (!(win))
		ERROR_EXIT("Failed to create window");
	SDL_Surface *surf = SDL_GetWindowSurface(win);
	if (!(surf))
		ERROR_EXIT("Failed to create SDL surface");
	SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, 0, 0, 0));
	SDL_UpdateWindowSurface(win);
	return win;
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

void draw_screen(const chip8_vm *vm, SDL_Window *win)
{
	SDL_Surface *screen = SDL_CreateRGBSurfaceWithFormatFrom(
		(void *)vm->screen, SCREEN_WIDTH, SCREEN_HEIGHT,
		24, // bit depth of 24
		SCREEN_WIDTH * 4, SDL_GetWindowPixelFormat(win));
	SDL_BlitSurface(screen, NULL, SDL_GetWindowSurface(win), NULL);
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

void update_vm_keyboard(chip8_vm *vm)
{
	const uint8_t *keyboard = SDL_GetKeyboardState(NULL);
	if (!keyboard)
		ERROR_EXIT("Failed to get keyboard state array");
	uint8_t keystates[] = {
		keyboard[SDL_SCANCODE_M], keyboard[SDL_SCANCODE_6],
		keyboard[SDL_SCANCODE_7], keyboard[SDL_SCANCODE_8],
		keyboard[SDL_SCANCODE_Y], keyboard[SDL_SCANCODE_U],
		keyboard[SDL_SCANCODE_I], keyboard[SDL_SCANCODE_H],
		keyboard[SDL_SCANCODE_J], keyboard[SDL_SCANCODE_K],
		keyboard[SDL_SCANCODE_N], keyboard[SDL_SCANCODE_COMMA],
		keyboard[SDL_SCANCODE_9], keyboard[SDL_SCANCODE_O],
		keyboard[SDL_SCANCODE_L], keyboard[SDL_SCANCODE_PERIOD],
	};
	memcpy(vm->keyboard, &keystates, LEN(keystates));
}

void main_loop(chip8_vm *vm, SDL_Window *win)
{
	SDL_Event event;
	struct timespec frame_start;
	struct timespec frame_end;
	struct timespec delay = { 0, 0 };
	long delta;
	long timers_last_decremented = 0;
	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &frame_start);
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return;
			default:
				continue;
			}
		}
		update_vm_keyboard(vm);
		vm_cycle(vm);
		// TODO: add audio
		if (vm->draw_flag) {
			draw_screen(vm, win);
			SDL_UpdateWindowSurface(win);
			vm->draw_flag = false;
		}
		clock_gettime(CLOCK_MONOTONIC, &frame_end);
		delta = ZERO_FLOOR(frame_end.tv_nsec - frame_start.tv_nsec);
		if (timers_last_decremented += delta >= TIMER_HZ_NS) {
			vm->delay_timer = ZERO_FLOOR(vm->delay_timer - 1);
			vm->sound_timer = ZERO_FLOOR(vm->sound_timer - 1);
			timers_last_decremented = 0;
		}
		delay.tv_nsec =
			ZERO_FLOOR(frame_end.tv_nsec - frame_start.tv_nsec);
		if (delay.tv_nsec)
			nanosleep(&delay, NULL);
	}
}

