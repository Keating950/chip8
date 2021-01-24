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
	bool key_pressed = false;
	int ops_since_draw = 0;
	bool paused = false;

	for (;;) {
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
				} else {
					key_pressed = true; // FALLTHROUGH
				}
			case SDL_KEYUP:
				update_vm_keyboard(vm);
			}
		}
		if (paused) {
			usleep(250000); // quarter-second
			continue;
		}
		vm_cycle(vm, key_pressed);
		key_pressed = false;

		if (ops_since_draw++ == OPS_PER_FRAME) {
			ops_since_draw = 0;
			vm->delay_timer = ZERO_FLOOR(vm->delay_timer - 1);
			vm->sound_timer = ZERO_FLOOR(vm->sound_timer - 1);
			draw_screen(vm);
			clock_gettime(CLOCK_MONOTONIC, &frame_end);
			delta = ZERO_FLOOR((TIMER_HZ_NS - difftime_ns(&frame_start, &frame_end))) / 1000;
			if (delta)
				usleep(delta);
		}
	}
}

inline long difftime_ns(const struct timespec *then, const struct timespec *now)
{
	const long one_sec = 1000000000;
	long nsec_diff = now->tv_nsec - then->tv_nsec;
	return nsec_diff > 0 ? nsec_diff : nsec_diff + one_sec;
}

void draw_screen(const chip8_vm *vm)
{
	const SDL_Rect dest = { .x = 0, .y = 0, .w = SCREEN_WIDTH, .h = SCREEN_HEIGHT };
	const SDL_Rect src = {
		.x = 0, .y = 0, .w = SCREEN_WIDTH / RENDER_SCALE, .h = SCREEN_HEIGHT / RENDER_SCALE
	};
	if (SDL_UpdateTexture(vm_texture, &src, (void *)vm->screen, 0x40 * sizeof(uint32_t)))
		DIE(SDL_GetError());
	SDL_RenderCopy(renderer, vm_texture, &src, &dest);
	SDL_RenderPresent(renderer);
}

void sdl_init(void)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
		DIE("Could not initialize SDL");
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &win, &renderer);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
	if (!(win) || !(renderer))
		DIE(SDL_GetError());
	vm_texture = SDL_CreateTexture(renderer, SDL_GetWindowPixelFormat(win),
								   SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	if (!vm_texture || SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF) ||
		SDL_RenderClear(renderer))
		DIE(SDL_GetError());
	SDL_RenderPresent(renderer);
}

void update_vm_keyboard(chip8_vm *vm)
{
	const SDL_Scancode keys[] = {
		CHIP8_KEY_0, CHIP8_KEY_1, CHIP8_KEY_2, CHIP8_KEY_3, CHIP8_KEY_4, CHIP8_KEY_5,
		CHIP8_KEY_6, CHIP8_KEY_7, CHIP8_KEY_8, CHIP8_KEY_9, CHIP8_KEY_A, CHIP8_KEY_B,
		CHIP8_KEY_C, CHIP8_KEY_D, CHIP8_KEY_E, CHIP8_KEY_F,
	};
	const uint8_t *kbd_state = SDL_GetKeyboardState(NULL);
	for (int i = 0x0; i < 0xF; i++)
		vm->keyboard[i] = kbd_state[keys[i]];
}
