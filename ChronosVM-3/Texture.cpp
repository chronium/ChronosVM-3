#include "Texture.h"



Texture::Texture (SDL_Renderer *renderer, int width, int height) :
	width (width),
	height (height),
	renderer (renderer) {
	pixels = new uint32_t[this->GetSize ()];
	SDL_memset4 (pixels, 0xFFFFFFFF, this->GetSize ());

	this->texture = SDL_CreateTexture (this->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, this->width, this->height);
	this->Update ();
}

Texture::~Texture () {
	delete[] this->pixels;
	SDL_DestroyTexture (this->texture);
}

void Texture::Draw (SDL_Rect *dest, SDL_Rect *src) {
	SDL_RenderCopy (this->renderer, this->texture, src, dest);
}

void Texture::Update (SDL_Rect *region) {
	SDL_UpdateTexture (this->texture, region, this->pixels, this->width * sizeof (uint32_t));
}
