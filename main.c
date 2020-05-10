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

SDL_Window *init_window(void);
int init_audio(void);
int scancode_to_chip8(int scancode);
static void catch_exit(int signo);
void draw_screen(const chip8_vm *vm, SDL_Window *win);
void main_loop(chip8_vm *vm, SDL_Window *win);
static inline void difftime_ns(const struct timespec *then, 
		const struct timespec *now, struct timespec *result);

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
	int cycles_since_timer_tick;
	int key;
	int key_pressed = 0;
	
	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &frame_start);
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN:
				key_pressed = 1;
			case SDL_KEYUP: {
				key = scancode_to_chip8(event.key.keysym.scancode);
				vm->keyboard[key] = !vm->keyboard[key];
			}
			default:
				continue;
			}
		}
		vm_cycle(vm, key_pressed);
		key_pressed = 0;
		// TODO: add audio
		if (vm->draw_flag) {
			draw_screen(vm, win);
			SDL_UpdateWindowSurface(win);
			vm->draw_flag = 0;
		}

		if (++cycles_since_timer_tick==8) {
			vm->delay_timer = ZERO_FLOOR(vm->delay_timer-1);
			vm->sound_timer = ZERO_FLOOR(vm->sound_timer-1);
			cycles_since_timer_tick=0;
		}
		
		clock_gettime(CLOCK_MONOTONIC, &frame_end);
		difftime_ns(&frame_start, &frame_end, &delay);
		if (delay.tv_nsec)
			nanosleep(&delay, NULL);
	}
}

static inline void 
difftime_ns(const struct timespec *then, const struct timespec *now, 
		struct timespec *result)
{
    if ((now->tv_nsec - then->tv_nsec) < 0) {
        result->tv_nsec = now->tv_nsec - then->tv_nsec + 1000000000;
    } else {
        result->tv_nsec = now->tv_nsec - then->tv_nsec;
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

