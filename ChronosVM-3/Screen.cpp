#include "Screen.h"

#include <thread>

Screen::Screen (VirtualMachine *VM): 
	Hardware (VM),
	MemoryRegion (0xA0000, (160*25) + (256 * 4)) {
	this->data = new uint8_t[160 * 25 * 2];
}

Screen::~Screen () { }

void Screen::Start () {
	Hardware::GetVM ()->RequestPortInb (0x00, [&] (uint8_t value) {
		this->scale = value;
	});
	Hardware::GetVM ()->RequestPortInb (0x01, [&] (uint8_t value) {
		if (value == 0x01) {
			if (this->window == nullptr) {
				std::thread ([&] {SDL SDL (SDL_INIT_VIDEO);

				this->window = new SDLWindow (Hardware::GetVM (), 640 * this->scale, 400 * this->scale, "Virtual Screen");
				this->texture = new Texture (this->window->GetRenderer (), 640, 400);

				auto updateFunc = [&] () { this->texture->Update (); };
				auto renderFunc = [&] (SDL_Renderer *renderer) {
					this->texture->Draw ();
				};

				this->window->EnterLoop (updateFunc, renderFunc); }).detach ();
			}
		}
	});
	Hardware::GetVM ()->RequestPortInb (0x02, [&] (uint8_t value) {
		while (this->texture == nullptr);
		this->texture->Clear (((uint32_t *) (this->palette))[value]);
	});

	Hardware::GetVM ()->RequestPortOutb (0x0A, [&] () {
		VirtualMachine *VM = Hardware::GetVM ();
		uint8_t state = VM->keyboard.front();
		VM->keyboard.pop ();
		return state;
	});
}

void Screen::putpixel (uint32_t x, uint32_t y, uint8_t color) {
	this->texture->GetPixels32 ()[x + y * this->texture->GetWidth ()] = ((uint32_t *) palette)[color];
}

void Screen::draw_glyph (uint8_t glyph, uint32_t sx, uint32_t sy, uint8_t color) {
	int i;
	unsigned int y;
	unsigned int glline;

	sx *= 8;
	sy *= 16;

	for (y = 0; y < FONT_SCANLINES; y++) {
		glline = fb_font[glyph * FONT_SCANLINES + y];
		for (i = 0; i < 8; i++) {
			if (glline & (1 << (7 - i)))
				putpixel (sx + i, sy + y, color & 0x0F);
			else
				putpixel (sx + i, sy + y, (color & 0xF0) >> 4);
		}

	}
}


void Screen::writew (uint32_t absolute, uint32_t relative, uint8_t data) {
	if (relative >= 0x400) {
		while (this->texture == nullptr);
		uint32_t pos = relative - 0x400;
		this->data[pos] = data;
		if (pos % 2 == 1)
			draw_glyph (data, (pos % 160) / 2, (pos / 160), this->data[pos - 1]);
	}
	else
		palette[relative] = data;
}
void Screen::writed (uint32_t absolute, uint32_t relative, uint16_t data) {
	this->writew (absolute, relative, (data & 0x00FF));
	this->writew (absolute + 1, relative + 1, (data & 0xFF00) >> 8);
}
void Screen::writeq (uint32_t absolute, uint32_t relative, uint32_t data) {
	this->writed (absolute, relative, (data & 0x0000FFFF));
	this->writed (absolute + 2, relative + 2, (data & 0xFFFF0000) >> 16);
}
