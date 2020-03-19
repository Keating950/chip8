//
// Created by bigfootlives on 2020-03-19.
//

#ifndef CHIP8_AV_IO_H
#define CHIP8_AV_IO_H
#include <SDL.h>
#include "chip8.h"

void init_window(SDL_Window **win, SDL_Surface **surf);
int init_audio();
void draw_screen(chip8_vm *vm, SDL_Surface **win_surf);
#endif //CHIP8_AV_IO_H
