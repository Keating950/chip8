//
// Created by bigfootlives on 2020-03-09.
//

#ifndef SDL_HELPERS_H
#define SDL_HELPERS_H

enum screen_dimensions {
	SCREEN_WIDTH = 640,
	SCREEN_HEIGHT = 320,
};

void sdl_error_exit();
void init_window(SDL_Window **win, SDL_Surface **surf);
SDL_Surface *load_image(const char *path);

#endif //SDL_HELPERS_H
