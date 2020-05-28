#ifdef __APPLE__
#include <sys/cdefs.h>
#endif
#define _BSD_SOURCE
#include <SDL.h>
#include <SDL_video.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "chip8.h"
#include "util.h"

#define TIMER_HZ_NS 16666667
#define OPS_PER_FRAME 8
#define SCALE 10
#define SCREEN_WIDTH 0x40 * SCALE
#define SCREEN_HEIGHT 0x20 * SCALE

inline long difftime_ns(const struct timespec *then, const struct timespec *now);
void draw_screen(const chip8_vm *vm);
void main_loop(chip8_vm *vm);
void sdl_init(void);
void sdl_cleanup(void);
void update_vm_keyboard(chip8_vm *vm);

static SDL_Window *win = NULL;
static int winfmt = 0;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *vm_texture = NULL;

// argc, argv format required by SDL
int main(int argc, char **argv)
{
	if (argc < 2) 
		DIE("Wrong number of arguments.\n"
			"Usage:\n\tchip8 [path to rom]\n");
	sdl_init();
	atexit(sdl_cleanup);
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
	int ops_since_draw = 0;
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
				update_vm_keyboard(vm);
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
		goto cleanup;
	if (SDL_UpdateTexture(vm_texture, &src, (void *)vm->screen,
						  0x40 * sizeof(uint32_t)))
		goto cleanup;
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderCopy(renderer, vm_texture, &src, &dest);
	SDL_RenderPresent(renderer);
	return;

cleanup:
	if (vm_texture)
		SDL_DestroyTexture(vm_texture);
	DIE(SDL_GetError());
}

void sdl_cleanup(void)
{
	if (win)
		SDL_DestroyWindow(win);
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (vm_texture)
		SDL_DestroyTexture(vm_texture);
}

void sdl_init(void)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
		DIE("Could not initialize SDL");
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN,
								&win, &renderer);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
	if (!(win) || !(renderer))
		goto cleanup;
	vm_texture = SDL_CreateTexture(renderer, winfmt, SDL_TEXTUREACCESS_STREAMING,
								   SCREEN_WIDTH, SCREEN_HEIGHT);
	if (!vm_texture)
		goto cleanup;
	winfmt = SDL_GetWindowPixelFormat(win);
	if (SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF))
		goto cleanup;
	if (SDL_RenderClear(renderer))
		goto cleanup;
	SDL_RenderPresent(renderer);
	return;

cleanup:
	sdl_cleanup();
	DIE(SDL_GetError());
}

void update_vm_keyboard(chip8_vm *vm)
{
	const uint8_t *kbd_state = SDL_GetKeyboardState(NULL);
	vm->keyboard[0x0] = kbd_state[SDL_SCANCODE_X];
	vm->keyboard[0x1] = kbd_state[SDL_SCANCODE_1];
	vm->keyboard[0x2] = kbd_state[SDL_SCANCODE_2];
	vm->keyboard[0x3] = kbd_state[SDL_SCANCODE_3];
	vm->keyboard[0x4] = kbd_state[SDL_SCANCODE_Q];
	vm->keyboard[0x5] = kbd_state[SDL_SCANCODE_W];
	vm->keyboard[0x6] = kbd_state[SDL_SCANCODE_E];
	vm->keyboard[0x7] = kbd_state[SDL_SCANCODE_A];
	vm->keyboard[0x8] = kbd_state[SDL_SCANCODE_S];
	vm->keyboard[0x9] = kbd_state[SDL_SCANCODE_D];
	vm->keyboard[0xA] = kbd_state[SDL_SCANCODE_Z];
	vm->keyboard[0xB] = kbd_state[SDL_SCANCODE_C];
	vm->keyboard[0xC] = kbd_state[SDL_SCANCODE_4];
	vm->keyboard[0xD] = kbd_state[SDL_SCANCODE_R];
	vm->keyboard[0xE] = kbd_state[SDL_SCANCODE_F];
	vm->keyboard[0xF] = kbd_state[SDL_SCANCODE_V];
}
