#include <iostream>
#include <SDL.h>
#include "SDL_image.h"
#include "components.h"
#include "sdl_helpers.h"

chip8 hardware;

void hello_world_sdl()
{
    auto[window, screen_surface] = sdl_setup();
    //Fill the surface white
    SDL_FillRect(screen_surface,
                 nullptr,
                 SDL_MapRGB(screen_surface->format, 0x00, 0x00, 0x00)
    );

    auto hello_world = load_image("helloworld.png");
    SDL_BlitSurface(hello_world, nullptr, screen_surface, nullptr);
    //Update the surface
    SDL_UpdateWindowSurface(window);
    // pump the message/event queue
    SDL_Event event;
    while (SDL_PollEvent(&event));
    //Wait two seconds
    SDL_Delay(2000);
    SDL_DestroyWindow(window);
    //Quit SDL subsystems
    SDL_Quit();
}


// argc, argv format required by SDL
int main(int argc, char **argv)
{
    hello_world_sdl();
    return 0;
}
