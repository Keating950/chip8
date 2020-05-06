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
int init_audio(void);
int scancode_to_chip8(int scancode);
static void catch_exit(int signo);
void draw_screen(const chip8_vm *vm, SDL_Window *win);
void main_loop(chip8_vm *vm, SDL_Window *win);

jmp_buf env;

// argc, argv format required by SDL
int main(int argc, char **argv)
{
	int jumpval;
	if (argc < 2) {
		fprintf(stderr, "Wrong number of arguments.\n"
						"Usage:\n\tchip8 [path to rom]\n");
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, catch_exit);
	signal(SIGTERM, catch_exit);
	signal(SIGQUIT, catch_exit);
	signal(SIGHUP, catch_exit);
	SDL_Window *win = init_window();
	chip8_vm vm = init_chip8();
	load_rom(argv[1], &vm);
	jumpval = setjmp(env);
	if (!(jumpval))
		main_loop(&vm, win);
	SDL_DestroyWindow(win);
	exit(EXIT_SUCCESS);
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
			case SDL_KEYDOWN:
				vm->keyboard[scancode_to_chip8(event.key.keysym.scancode)] = 1;
				break;
			case SDL_KEYUP:
				vm->keyboard[scancode_to_chip8(event.key.keysym.scancode)] = 0;
			default:
				continue;
			}
		}
		vm_cycle(vm);
		// TODO: add audio
		if (vm->draw_flag) {
			draw_screen(vm, win);
			SDL_UpdateWindowSurface(win);
			vm->draw_flag = 0;
		}
		clock_gettime(CLOCK_MONOTONIC, &frame_end);
		delta = ZERO_FLOOR(frame_end.tv_nsec - frame_start.tv_nsec);
		if (timers_last_decremented += delta >= TIMER_HZ_NS) {
			vm->delay_timer = ZERO_FLOOR(vm->delay_timer - 1);
			vm->sound_timer = ZERO_FLOOR(vm->sound_timer - 1);
			timers_last_decremented = 0;
		}
		delay.tv_nsec = ZERO_FLOOR(frame_end.tv_nsec - frame_start.tv_nsec);
		if (delay.tv_nsec)
			nanosleep(&delay, NULL);
	}
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
						 SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT,
						 SDL_WINDOW_SHOWN);
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
		(void *)vm->screen, SCREEN_WIDTH, SCREEN_HEIGHT, 24, // bit depth
		SCREEN_WIDTH * 4, SDL_GetWindowPixelFormat(win));
	SDL_BlitSurface(screen, NULL, SDL_GetWindowSurface(win), NULL);
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

