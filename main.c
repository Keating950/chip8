#ifdef __APPLE__
#include <sys/cdefs.h>
#endif
#define _BSD_SOURCE
#include <SDL.h>
#include <SDL_video.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "chip8.h"
#include "util.h"

#define TIMER_HZ_NS 16666667
#define OPS_PER_FRAME 8
#define SCREEN_WIDTH 0x40 * RENDER_SCALE
#define SCREEN_HEIGHT 0x20 * RENDER_SCALE

long difftime_ns(const struct timespec *then, const struct timespec *now);
void draw_screen(const chip8_vm *vm);
void main_loop(chip8_vm *vm);
void sdl_init(void);
void update_vm_keyboard(chip8_vm *vm);

static SDL_Window *win = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *vm_texture = NULL;

// argc, argv format required by SDL
int main(int argc, char **argv)
{
	if (argc < 2) {
		fputs("Wrong number of arguments.\n"
			  "Usage:\n\tchip8 [path to rom]\n",
			  stderr);
		exit(EXIT_FAILURE);
	}
	sdl_init();
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
	int delta;
	int key_pressed = 0;
	int ops_since_draw = 0;
	int paused = 0;

	while (1) {
		if (!ops_since_draw)
			clock_gettime(CLOCK_MONOTONIC, &frame_start);
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN:
				if (event.key.keysym.scancode == PAUSE_KEY) {
					ops_since_draw = 0;
					clock_gettime(CLOCK_MONOTONIC, &frame_start);
					paused = !paused;
					break;
				} else
					key_pressed = 1; // FALLTHROUGH
			case SDL_KEYUP:
				update_vm_keyboard(vm);
			default:
				continue;
			}
		}
		if (paused) {
			usleep(250000); // quarter-second
			continue;
		}
		vm_cycle(vm, key_pressed);
		key_pressed = 0;
		// TODO: add audio

		if (ops_since_draw++ == OPS_PER_FRAME) {
			ops_since_draw = 0;
			vm->delay_timer = ZERO_FLOOR(vm->delay_timer - 1);
			vm->sound_timer = ZERO_FLOOR(vm->sound_timer - 1);
			draw_screen(vm);
			clock_gettime(CLOCK_MONOTONIC, &frame_end);
			delta =
				ZERO_FLOOR((TIMER_HZ_NS - difftime_ns(&frame_start, &frame_end)))
				/ 1000;
			if (delta)
				usleep(delta);
		}
	}
}

inline long difftime_ns(const struct timespec *then, const struct timespec *now)
{
	if ((now->tv_nsec - then->tv_nsec) < 0) {
		return now->tv_nsec - then->tv_nsec + 1000000000;
	} else {
		return now->tv_nsec - then->tv_nsec;
	}
}

void draw_screen(const chip8_vm *vm)
{
	static const SDL_Rect dest = {
		.x = 0, .y = 0, .w = SCREEN_WIDTH, .h = SCREEN_HEIGHT
	};
	static const SDL_Rect src = { .x = 0, .y = 0, .w = 0x40, .h = 0x20 };
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	if (SDL_RenderClear(renderer))
		DIE(SDL_GetError());
	if (SDL_UpdateTexture(vm_texture, &src, (void *)vm->screen,
						  0x40 * sizeof(uint32_t)))
		DIE(SDL_GetError());
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderCopy(renderer, vm_texture, &src, &dest);
	SDL_RenderPresent(renderer);
}

void sdl_init(void)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
		DIE("Could not initialize SDL");
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN,
								&win, &renderer);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
	if (!(win) || !(renderer))
		DIE(SDL_GetError());
	vm_texture = SDL_CreateTexture(renderer, SDL_GetWindowPixelFormat(win),
								   SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH,
								   SCREEN_HEIGHT);
	if (!vm_texture)
		DIE(SDL_GetError());
	if (SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF))
		DIE(SDL_GetError());
	if (SDL_RenderClear(renderer))
		DIE(SDL_GetError());
	SDL_RenderPresent(renderer);
}

void update_vm_keyboard(chip8_vm *vm)
{
	const uint8_t *kbd_state = SDL_GetKeyboardState(NULL);
	vm->keyboard[0x0] = kbd_state[CHIP8_KEY_0];
	vm->keyboard[0x1] = kbd_state[CHIP8_KEY_1];
	vm->keyboard[0x2] = kbd_state[CHIP8_KEY_2];
	vm->keyboard[0x3] = kbd_state[CHIP8_KEY_3];
	vm->keyboard[0x4] = kbd_state[CHIP8_KEY_4];
	vm->keyboard[0x5] = kbd_state[CHIP8_KEY_5];
	vm->keyboard[0x6] = kbd_state[CHIP8_KEY_6];
	vm->keyboard[0x7] = kbd_state[CHIP8_KEY_7];
	vm->keyboard[0x8] = kbd_state[CHIP8_KEY_8];
	vm->keyboard[0x9] = kbd_state[CHIP8_KEY_9];
	vm->keyboard[0xA] = kbd_state[CHIP8_KEY_A];
	vm->keyboard[0xB] = kbd_state[CHIP8_KEY_B];
	vm->keyboard[0xC] = kbd_state[CHIP8_KEY_C];
	vm->keyboard[0xD] = kbd_state[CHIP8_KEY_D];
	vm->keyboard[0xE] = kbd_state[CHIP8_KEY_E];
	vm->keyboard[0xF] = kbd_state[CHIP8_KEY_F];
}
