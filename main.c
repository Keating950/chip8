#include "SDL_image.h"
#include "chip8.h"
#include "sdl_helpers.h"
#include <SDL.h>
#include <SDL_video.h>
#include <stdio.h>

void hello_world_sdl()
{
	SDL_Window *window = NULL;
	SDL_Surface *screen_surface = NULL;
	init_window(&window, &screen_surface);
	SDL_Surface *hello_world = load_image("helloworld.png");
	SDL_BlitSurface(hello_world, NULL, screen_surface, NULL);
	// Update the surface
	SDL_UpdateWindowSurface(window);
	// pump the message/event queue
	SDL_Event event;
	while (SDL_PollEvent(&event))
		;
	// Wait two seconds
	SDL_Delay(2000);
	SDL_DestroyWindow(window);
	// Quit SDL subsystems
	SDL_Quit();
}

void print_rom(chip8_vm vm)
{
	do {
		for (int i = 0; i < 3; i++) {
			unsigned short opcode =
				vm.rom[vm.pc] << 8 | vm.rom[vm.pc + 1];
			unsigned short instruction = opcode & 0xf000;
			printf("%#X  ", instruction);
			vm.pc += 2;
		}
		printf("\n");
	} while (vm.pc < 800);
}

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

	chip8_vm vm = initialize_chip8();
	load_rom(argv[1], &vm);
	print_rom(vm);
	return 0;
}