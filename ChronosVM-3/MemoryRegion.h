#pragma once

#include <stdint.h>
#include <functional>

class MemoryRegion {
public:
	MemoryRegion (uint32_t start, uint32_t size);
	~MemoryRegion ();

	uint32_t GetAddress () { return this->start; }
	uint32_t GetSize () { return this->size; }
	uint32_t GetEnd () { return this->start + this->size; }

	bool ContainsAddress (uint32_t address) { return (address >= this->GetAddress ()) && (address < this->GetEnd ()); }

	virtual void writew (uint32_t absolute, uint32_t relative, uint8_t data) { }
	virtual void writed (uint32_t absolute, uint32_t relative, uint16_t data) { }
	virtual void writeq (uint32_t absolute, uint32_t relative, uint32_t data) { }

	virtual uint8_t readw (uint32_t absolute, uint32_t relative) { return 0; }
	virtual uint16_t readd (uint32_t absolute, uint32_t relative) { return 0; }
	virtual uint32_t readq (uint32_t absolute, uint32_t relative) { return 0; }

private:
	uint32_t start;
	uint32_t size;
};

