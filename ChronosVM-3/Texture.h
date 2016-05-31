#pragma once

#include <SDL.h>

class Texture {
public:
	Texture (SDL_Renderer *renderer, int width, int height);
	~Texture ();

	void Update (SDL_Rect *region = NULL);
	void Draw (SDL_Rect *dest = NULL, SDL_Rect *src = NULL);
	
	inline void Clear (uint32_t color) const { SDL_memset4 (this->pixels, color, this->GetSize ()); }

	inline uint32_t *GetPixels32 () const { return this->pixels; }
	inline uint16_t *GetPixels16 () const { return (uint16_t *) this->pixels; }
	inline uint8_t *GetPixels8 () const { return (uint8_t *) this->pixels; }

	inline int GetWidth () const { return this->width; }
	inline int GetHeight () const { return this->height; }

	inline uint32_t GetSize () const { return this->GetWidth () * this->GetHeight (); }
private:
	int width;
	int height;

	SDL_Texture *texture;
	SDL_Renderer *renderer;

	uint32_t *pixels;
};

