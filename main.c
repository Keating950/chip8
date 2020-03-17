#define _GNU_SOURCE
#include "SDL_image.h"
#include "chip8.h"
#include "sdl_helpers.h"
#include <SDL.h>
#include <SDL_video.h>
#include <stdio.h>
#include <search.h>
#define handle_error(msg)                                                      \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wint-to-pointer-cast"
struct hsearch_data *create_key_htab()
{
	/* FLAG: Narrowing conversion!
	  SDLK macros are ints. However, with my
	  control scheme, they should not overflow. */
	ENTRY e, *ep;
	int sdl_keys[0x10]
		= { SDLK_y, SDLK_u, SDLK_i, SDLK_o, SDLK_h,	SDLK_j,
		    SDLK_k, SDLK_l, SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD };
	struct hsearch_data *htab = calloc(23, sizeof(struct hsearch_data));

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
		hsearch_r(e, ENTER, &ep, htab);
		if (!(ep))
			handle_error("Couldn't add key to hashtable");
	}
	return htab;
}
#pragma clang diagnostic pop

void main_loop(chip8_vm vm)
{
	SDL_Window *win;
	SDL_Surface *surf;
	SDL_Event e;
	init_window(&win, &surf);

	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT)
			break;
		else if (e.type == SDL_KEYDOWN) {
			switch (e.key.keysym.sym) {
			case SDLK_y:
				break;
			case SDLK_u:
				break;
			case SDLK_i:
				break;
			case SDLK_o:
				break;
			case SDLK_p:
				break;
			case SDLK_h:
				break;
			case SDLK_j:
				break;
			case SDLK_k:
				break;
			case SDLK_l:
				break;
			case SDLK_n:
				break;
			case SDLK_m:
				break;
			case SDLK_COMMA:
				break;
			case SDLK_PERIOD:
				break;
			default:
				break;
			}
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

	struct hsearch_data *htab = create_key_htab();
	chip8_vm vm = initialize_chip8();
	load_rom(argv[1], &vm);
	vm_cycle(&vm);
	//    SDL_Window *win;
	//    SDL_Surface *surf;
	//    init_window(&win, &surf);
	return 0;
}
