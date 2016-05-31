#pragma once

#include <stdint.h>

#include <SDL.h>
#include "InitError.h"

class SDL {
public:
	SDL (uint32_t flags = 0) ;
	virtual ~SDL ();
};

