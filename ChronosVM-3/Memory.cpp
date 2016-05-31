#include "Memory.h"



Memory::Memory (uint32_t size) :
	size (size) {
	memory = new uint8_t[this->size];
}


Memory::~Memory () { 
	delete[] memory;
}

#define TryWriteMMIO(addr, val, t) for (auto *region : this->mmio) \
									if (region->ContainsAddress (addr)) { \
										region->write##t## (addr, addr - region->GetAddress (), val); \
										return; \
									}
#define TryReadMMIO(addr, t) for (auto *region : this->mmio) \
									if (region->ContainsAddress (addr)) \
										return region->read##t## (addr, addr - region->GetAddress ());

void Memory::writew (uint32_t addr, uint8_t val) {
	TryWriteMMIO (addr, val, w)
									else {
		this->memory[addr] = val;
	}
}
void Memory::writed (uint32_t addr, uint16_t val) {
	TryWriteMMIO (addr, val, d)
									else {
		this->writew (addr + 0, (val & 0x00FF) >> 0);
		this->writew (addr + 1, (val & 0xFF00) >> 8);
	}
}
void Memory::writeq (uint32_t addr, uint32_t val) {
	TryWriteMMIO (addr, val, q)
									else {
		this->writed (addr + 0, (val & 0x0000FFFF) >> 00);
		this->writed (addr + 2, (val & 0xFFFF0000) >> 16);
	}
}

uint8_t Memory::readw (uint32_t addr) {
	TryReadMMIO (addr, w);

	return this->memory[addr];
}
uint16_t Memory::readd (uint32_t addr) {
	TryReadMMIO (addr, d);

	return this->readw (addr) | this->readw (addr + 1) << 8;
}
uint32_t Memory::readq (uint32_t addr) {
	TryReadMMIO (addr, q);

	return this->readd (addr) | this->readd (addr + 2) << 16;
}
