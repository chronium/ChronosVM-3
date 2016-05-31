#pragma once

#include "Hardware.h"
#include "MemoryRegion.h"

#include <stdlib.h>

class Storage : public Hardware, public MemoryRegion {
public:
	Storage (VirtualMachine *VM);
	~Storage ();

	void Start ();
	void Stop ();

	void writew (uint32_t absolute, uint32_t relative, uint8_t data);
	void writed (uint32_t absolute, uint32_t relative, uint16_t data);
	void writeq (uint32_t absolute, uint32_t relative, uint32_t data);

	uint8_t readw (uint32_t absolute, uint32_t relative);
	uint16_t readd (uint32_t absolute, uint32_t relative);
	uint32_t readq (uint32_t absolute, uint32_t relative);
private:
	uint8_t sector = 0;
	uint8_t lane = 0;

	uint8_t *memory;
};

