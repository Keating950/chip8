#define _GNU_SOURCE
#include "SDL_image.h"
#include "chip8.h"
#include "sdl_helpers.h"
#include <SDL.h>
#include <SDL_video.h>
#include <stdio.h>
#include <search.h>
#include <time.h>

#ifdef __MACH__
timing_mach_init();
#endif

#define handle_error(msg)                                                      \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)

void create_key_htab()
{
	/* FLAG: Narrowing conversion!
	  SDLK macros are ints. However, with my
	  control scheme, they should not overflow. */
	ENTRY e, *ep;
	char sdl_keys[0x10]
		= { SDLK_y, SDLK_u, SDLK_i, SDLK_o, SDLK_h,	SDLK_j,
		    SDLK_k, SDLK_l, SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD };

	if (hcreate(23) == 0) { // zero on error -- it's weird
		perror("Error creating keyboard hashtable");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < 0x10; i++) {
		char *sdl_key = malloc(sizeof(char));
		char *chip8_key = malloc(sizeof(char));
		*sdl_key = sdl_keys[i];
		*chip8_key = i;
		e.key = sdl_key;
		e.data = chip8_key;
		ep = hsearch(e, ENTER);
		if (!(ep))
			handle_error("Couldn't add key to hashtable");
	}
}

void main_loop(chip8_vm *vm)
{
	SDL_Window *win;
	SDL_Surface *surf;
	SDL_Event event;
	struct ENTRY *ht_entry = { 0, 0 };
	struct timespec delay_ts;
	struct timespec sound_ts;
	struct timespec current_ts;

	clock_gettime(CLOCK_MONOTONIC, &delay_ts);
	clock_gettime(CLOCK_MONOTONIC, &sound_ts);
	init_window(&win, &surf);
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT)
			break;
		else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
			ht_entry.key = event.key.keysym.sym;
			hsearch(ht_entry, FIND);
			vm->keyboard[ht_entry.data]
				= !(vm->keyboard[ht_entry.data]);
		}
		vm_cycle(vm);
		if (vm->draw_flag) {
			// draw stuff
		}
		clock_gettime(CLOCK_MONOTONIC, &current_ts);
		if (current_ts.tv_sec - delay_ts.tv_sec
		    >= vm->delay_timer / 60) {
			// delay stuff
			vm->delay_timer = 0;
		}
		if (current_ts.tv_sec - sound_ts.tv_sec
		    >= vm->sound_timer / 60) {
			// beep
			vm->sound_timer = 0;
		}
	}
	SDL_DestroyWindow(win);
	SDL_Quit();
}

// argc, argv format required by SDL
int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Wrong number of arguments.\n"
				"Usage:\tchip8 [path to rom]");
		exit(1);
	}

	create_key_htab();
	chip8_vm vm = initialize_chip8();
	load_rom(argv[1], &vm);
	vm_cycle(&vm);
	//	SDL_Window *win;
	//	SDL_Surface *surf;
	//	init_window(&win, &surf);
	return 0;
}
