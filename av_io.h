//
// Created by bigfootlives on 2020-03-19.
//

#ifndef CHIP8_AV_IO_H
#define CHIP8_AV_IO_H
#include <SDL.h>
#include "chip8.h"

void init_window(SDL_Window **win);
int init_audio();
void draw_screen(const chip8_vm *vm, SDL_Window **win);
#endif //CHIP8_AV_IO_H
