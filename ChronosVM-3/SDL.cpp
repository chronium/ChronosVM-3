#include "SDL.h"

SDL::SDL (uint32_t flags) {
	if (SDL_Init (flags) != 0)
		throw InitError ();
}


SDL::~SDL () {
	SDL_Quit ();
}
