#pragma once

#include <string>
#include <SDL.h>
#include <functional>
#include <thread>
#include <mutex>

#include "VirtualMachine.h"

class SDLWindow {
public:
	SDLWindow (VirtualMachine *VM, int width, int height, const std::string title);
	~SDLWindow ();

	void EnterLoop (std::function<void ()> update, std::function<void (SDL_Renderer *)> render);
	void SetQuality (bool nearest = false);

	void Update (std::function<void ()> func);
	void Render (std::function<void (SDL_Renderer *)> func);

	inline bool IsCloseRequested () const { return this->is_close_requested; }
	inline SDL_Renderer *GetRenderer () const { return this->renderer; }

	inline void Clear () const { SDL_RenderClear (this->GetRenderer ()); }
	inline void Present () const { SDL_RenderPresent (this->GetRenderer ()); }
private:
	int width;
	int height;

	std::string title;

	SDL_Window *window;
	SDL_Renderer *renderer;

	VirtualMachine *VM;

	bool is_close_requested;
};

