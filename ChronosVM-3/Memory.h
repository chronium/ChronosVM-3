#pragma once

#include <stdint.h>
#include <vector>

#include "MemoryRegion.h"

class Memory {
public:
	Memory (uint32_t size);
	~Memory ();

	inline void AddMemoryRegion (MemoryRegion *region) { this->mmio.push_back (region); }

	void writew (uint32_t, uint8_t);
	void writed (uint32_t, uint16_t);
	void writeq (uint32_t, uint32_t);

	uint8_t readw (uint32_t);
	uint16_t readd (uint32_t);
	uint32_t readq (uint32_t);

	uint8_t *memory;
private:
	uint32_t size;
	std::vector<MemoryRegion *> mmio;
};

