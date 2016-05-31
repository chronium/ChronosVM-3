#include "stdafx.h"

#include <iostream>

#include <SDL.h>

#include "SDL.h"
#include "Timer.h"
#include "SDLWindow.h"
#include "InitError.h"
#include "Texture.h"
#include "Memory.h"
#include "VirtualMachine.h"
#include "Hardware.h"
#include "Screen.h"
#include "Storage.h"

void func (uint64_t delta, uint64_t total) {

}

int main(int argc, char **argv) {
	try {
		srand (time (NULL));

		VirtualMachine *VM = new VirtualMachine (0xFFFFF);
		Screen *screen = new Screen (VM);
		VM->AddHardware (screen);
		VM->AddMemoryRegion (screen);
		Storage *storage = new Storage (VM);
		VM->AddHardware (storage);
		VM->AddMemoryRegion (storage);

		VM->Start (argv[1]);

		getchar ();

		return 0;
	}
	catch (const InitError &err) {
		fprintf (stderr, "Error while initializing SDL: %s\n", err.what ());
	}
    return 1;
}

