#include "Storage.h"

#include <cstdio>

#include <fstream>

Storage::Storage (VirtualMachine *VM) :
	Hardware (VM),
	MemoryRegion (0x70000, 512) { 
}

Storage::~Storage () { 
	free (this->memory);
}

void Storage::Start () {
	this->memory = (uint8_t *) malloc (512 * 256 * 256);
	memset (this->memory, 0x00, 512 * 256 * 256);
	std::ifstream file ("data.img", std::ios::in | std::ios::binary);
	if (file.is_open ()) {
		file.rdbuf ()->sgetn ((char *) this->memory, 512 * 256 * 256);
		file.close ();
	}
}

void Storage::Stop () {
	std::ofstream file ("data.img", std::ios::out | std::ios::binary | std::ios::trunc);
	if (file.is_open ()) {
		file.rdbuf ()->sputn ((char *) this->memory, 512 * 256 * 256);
		file.close ();
	}
}

void Storage::writew (uint32_t absolute, uint32_t relative, uint8_t data) {
	this->memory[relative + (sector * 256) + (lane * 256 * 256)] = data;
}
void Storage::writed (uint32_t absolute, uint32_t relative, uint16_t data) {
	this->writew (absolute + 0, relative + 0, (data & 0xFF00) >> 8);
	this->writew (absolute + 1, relative + 1, (data & 0x00FF) >> 0);
}
void Storage::writeq (uint32_t absolute, uint32_t relative, uint32_t data) {
	this->writed (absolute + 0, relative + 0, (data & 0xFFFF0000) >> 16);
	this->writed (absolute + 2, relative + 2, (data & 0x0000FFFF) >> 00);
}

uint8_t Storage::readw (uint32_t absolute, uint32_t relative) {
	return this->memory[relative + (sector * 256) + (lane * 256 * 256)];
}
uint16_t Storage::readd (uint32_t absolute, uint32_t relative) {
	return (this->readw (absolute, relative) << 8) | this->readw (absolute + 1, relative + 1);
}
uint32_t Storage::readq (uint32_t absolute, uint32_t relative) {
	return (this->readd (absolute, relative) << 16) | this->readd (absolute + 2, relative + 2);
}
