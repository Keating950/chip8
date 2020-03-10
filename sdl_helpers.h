//
// Created by bigfootlives on 2020-03-09.
//

#ifndef SDL_HELPERS_H
#define SDL_HELPERS_H

void sdl_error_exit();
SDL_Surface *load_image(const char *path);
std::pair<SDL_Window *, SDL_Surface *> sdl_setup();

enum screen_dimensions
{
    SCREEN_WIDTH = 640,
    SCREEN_HEIGHT = 320,
};

#endif //SDL_HELPERS_H
