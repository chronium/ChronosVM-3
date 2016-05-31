#include "SDLWindow.h"

SDLWindow::SDLWindow (VirtualMachine *VM, int width, int height, const std::string title) :
	width (width),
	height (height),
	title (title),
	is_close_requested (false),
	VM (VM) {
	this->window = SDL_CreateWindow (this->title.c_str (), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, this->width, this->height, NULL);
	this->renderer = SDL_CreateRenderer (this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
}

SDLWindow::~SDLWindow () {
	SDL_DestroyRenderer (this->renderer);
	SDL_DestroyWindow (this->window);
}

void SDLWindow::Update (std::function<void ()> func) {
	SDL_Event e;
	while (SDL_PollEvent (&e)) {
		if (e.type == SDL_QUIT) {
			this->is_close_requested = true;
			VM->status = VirtualMachine::Off;
		} else if (e.type == SDL_KEYDOWN) {
			if (VM->interrupts_enabled) {
				VM->QueueKeyState (1);
				VM->QueueKeyState (e.key.keysym.scancode & 0xFF);
			}
			VM->interrupt (17);
		} else if (e.type == SDL_KEYUP) {
			if (VM->interrupts_enabled) {
				VM->QueueKeyState (0);
				VM->QueueKeyState (e.key.keysym.scancode & 0xFF);
			}
			VM->interrupt (17);
		}
	}

	if (func != NULL)
		func ();
}
void SDLWindow::Render (std::function<void (SDL_Renderer *)> func) {
	this->Clear ();
	if (func != NULL)
		func (this->renderer);
	this->Present ();
}

void SDLWindow::EnterLoop (std::function<void ()> update, std::function<void (SDL_Renderer *)> render) {
	while (!this->IsCloseRequested ()) {
		Update (update);
		Render (render);
	}
}

void SDLWindow::SetQuality (bool nearest) {
	if (nearest)
		SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	else
		SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "linear");
}
