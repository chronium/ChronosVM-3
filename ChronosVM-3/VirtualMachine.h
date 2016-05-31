#pragma once

#include <stdint.h>

#include <vector>
#include <map>
#include <mutex>
#include <queue>

#include "Memory.h"
#include "VirtualMachine.h"
#include "MemoryRegion.h"
#include "Timer.h"

class Hardware;

class VirtualMachine {
public:
	typedef union reg {
		uint32_t qword;
		struct word {
			uint8_t LL;
			uint8_t LH;
			uint8_t HL;
			uint8_t HH;
		} word;
		struct dword {
			uint16_t L;
			uint16_t H;
		} dword;
	} register_t;

	struct registers {
		register_t A;
		register_t B;
		register_t C;
		register_t D;
		register_t E;
		register_t F;

		register_t W;
		register_t X;
		register_t Y;
		register_t Z;

		uint16_t Flags;
		uint16_t Clocks;

		uint32_t CS, DS, SS;

		uint32_t PC, SP, BP;
	};

	enum flag {
		None		= 0b00000000,
		Zero		= 0b00000001,
		Underflow	= 0b00000010,
		Overflow	= 0b00000100,
		Parity		= 0b00001000,
	};

	enum registers_enum {
		A, AH, AL, AHH, AHL, ALH, ALL,
		B, BH, BL, BHH, BHL, BLH, BLL,
		C, CH, CL, CHH, CHL, CLH, CLL,
		D, DH, DL, DHH, DHL, DLH, DLL,
		E, EH, EL, EHH, EHL, ELH, ELL,
		F, FH, FL, FHH, FHL, FLH, FLL,
		W, WH, WL, WHH, WHL, WLH, WLL,
		X, XH, XL, XHH, XHL, XLH, XLL,
		Y, YH, YL, YHH, YHL, YLH, YLL,
		Z, ZH, ZL, ZHH, ZHL, ZLH, ZLL,

		Flags, Clocks,
		CS, DS, SS,
		PC, SP, BP
	};

	enum machine_status {
		Off			= 0b00000000,
		On			= 0b00000001,
		Halted		= 0b00000010,
		Sleeping	= 0b00000100,
		Hybernating = 0b00001000,
		Breakpoint	= 0b00010000,
		Fault		= 0b00100000,
		DFault		= 0b01000000,
		TFault		= 0b10000000,
	};

	enum addressing_mode {
		ImmediateImmediate,
		ImmediateRegister,
		ImmediateIndirect,
		ImmediateRindirect,
		RegisterImmediate,
		RegisterRegister,
		RegisterIndirect,
		RegisterRindirect,
		IndirectImmediate,
		IndirectRegister,
		IndirectIndirect,
		IndirectRIndirect,
		RIndirectImmediate,
		RIndirectRegister,
		RIndirectIndirect,
		RIndirectRIndirect,
		Immediate,
		Register,
		Indirect,
		RIndirect
	};

	enum instructions {
		nop,
		mov,
		cmp,
		cmps,
		jmp, je, jne, jg, jge, jl, jle,
		call, calle, callne, callg, callge, calll, callle, 
		intr,
		ret, iret,
		cli, sti,
		inc, dec,
		add, sub, mul, div, mod,
		not, and, or, xor, shl, shr,
		push, pop, pusha, popa,
		adds, subs, muls, divs, mods,
		nots, ands, ors, xors, shls, shrs,
		mmset, mmcpy,
		inb, inw, inq,
		outb, outw, outq,
		ldidt,
		hlt
	};

	struct int_desc {
		uint32_t address;
	};

	enum size {
		word	= 0b00,
		dword	= 0b01,
		qword	= 0b10
	};

	VirtualMachine (uint32_t memorySize);
	~VirtualMachine ();

	inline void RequestPortInb (uint8_t port, std::function<void (uint8_t value)> func) {
		this->inpb[port] = func;
	}
	inline void RequestPortInd (uint8_t port, std::function<void (uint16_t value)> func) {
		this->inpd[port] = func;
	}
	inline void RequestPortInq (uint8_t port, std::function<void (uint32_t value)> func) {
		this->inpq[port] = func;
	}

	inline void RequestPortOutb (uint8_t port, std::function<uint8_t ()> func) {
		this->outpb[port] = func;
	}
	inline void RequestPortOutd (uint8_t port, std::function<uint16_t ()> func) {
		this->outpd[port] = func;
	}
	inline void RequestPortOutq (uint8_t port, std::function<uint32_t ()> func) {
		this->outpq[port] = func;
	}
	
	inline void QueueKeyState (uint8_t state) { this->keyboard.push (state); }

	void pushw (uint8_t);
	void pushd (uint16_t);
	void pushq (uint32_t);

	uint8_t popw ();
	uint16_t popd ();
	uint32_t popq ();

	void mpusha ();
	void mpopa ();

	uint32_t read_reg (uint8_t reg);
	void set_reg (uint8_t reg, uint32_t val);
	const char *reg_name (uint8_t reg);
	const char *addressing_name (addressing_mode mode);
	uint32_t read_val (size sz);
	uint32_t read_val_n (size sz, uint32_t addr);
	uint32_t read_reg_ind (size sz, uint8_t reg);
	void write_val (size sz, uint32_t addr);
	void write_reg (size sz, uint32_t addr, uint8_t reg);
	void write_reg_ind (size sz, uint8_t reg, uint8_t reg2);
	void write_reg_rind_val_ind (size sz, uint8_t reg, uint32_t);
	void write_reg_rind_val (size sz, uint8_t reg);
	void set_reg_reg (size sz, uint8_t, uint8_t);

	void minb (uint8_t port, uint8_t value);
	void mind (uint8_t port, uint16_t value);
	void minq (uint8_t port, uint32_t value);

	uint8_t moutb (uint8_t port);
	uint16_t moutd (uint8_t port);
	uint32_t moutq (uint8_t port);

	void Start (char *path);

	void AddHardware (Hardware *hardware);
	void AddMemoryRegion (MemoryRegion *region);

	typedef void (VirtualMachine::*instruction) ();

	std::queue<uint8_t> keyboard;

	struct registers *registers;
	int status;
	Memory *memory;
	instruction instructions[256];
	std::vector<Hardware *> hardware;

	static std::map<uint8_t, std::function<void (uint8_t)>> inpb;
	static std::map<uint8_t, std::function<void (uint16_t)>> inpd;
	static std::map<uint8_t, std::function<void (uint32_t)>> inpq;

	static std::map<uint8_t, std::function<uint8_t ()>> outpb;
	static std::map<uint8_t, std::function<uint16_t ()>> outpd;
	static std::map<uint8_t, std::function<uint32_t ()>> outpq;

	int_desc *int_descs;
	bool interrupts_enabled = false;

	bool inter = false;
	uint8_t int_line = 0;

	Timer *timer;
	std::mutex lock;

	void interrupt (uint8_t line);

	void unimplemented_instruction ();
	
	void NOP ();

	void MOV ();

	void CMP ();
	void JMP ();
	void JE ();	
	void JNE ();
	void JG ();	
	void JGE ();
	void JL ();	
	void JLE ();

	void CALL ();
	void CALLE ();

	void RET ();
	void IRET ();

	inline void CLI () { printf ("cli"); this->interrupts_enabled = false; }
	inline void STI () { printf ("sti"); this->interrupts_enabled = true; }

	void INC ();
	void DEC ();

	void ADD ();
	void SUB ();
	void MUL ();
	void DIV ();
	void MOD ();

	void NOT ();
	void AND ();
	void OR ();	
	void XOR ();

	void SHL ();
	void SHR ();

	void PUSH ();
	void POP ();
	void PUSHA ();
	void POPA ();

	void INB ();
	void INW ();
	void INQ ();

	void OUTB ();
	void OUTW ();
	void OUTQ ();

	void LDIDT ();

	void HLT ();
};

