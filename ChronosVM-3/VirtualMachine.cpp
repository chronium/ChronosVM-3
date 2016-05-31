#include "VirtualMachine.h"

#include "Hardware.h"

#include <fstream>

std::map<uint8_t, std::function<void (uint8_t)>> VirtualMachine::inpb;
std::map<uint8_t, std::function<void (uint16_t)>> VirtualMachine::inpd;
std::map<uint8_t, std::function<void (uint32_t)>> VirtualMachine::inpq;

std::map<uint8_t, std::function<uint8_t ()>> VirtualMachine::outpb;
std::map<uint8_t, std::function<uint16_t ()>> VirtualMachine::outpd;
std::map<uint8_t, std::function<uint32_t ()>> VirtualMachine::outpq;

#define has_in(port, t) this->inp##t##.count (port) > 0
#define inp(port, t, val) (this->inp##t##.at (port)) (val)

#define has_out(port, t) this->outp##t##.count (port) > 0
#define outp(port, t) (this->outp##t##.at (port)) ()

uint32_t colors[] {
	0xff1d1f21,
	0xff5f819d,
	0xff8c9440,
	0xff5e8d87,
	0xffa54242,
	0xff85678f,
	0xffde935f,
	0xff707880,
	0xff81a2be,
	0xffb5bd68,
	0xff8abeb7,
	0xffcc6666,
	0xffb294bb,
	0xfff0c674,
	0xffc5c8c6,
};

VirtualMachine::VirtualMachine (uint32_t memorySize) {
	this->memory = new Memory (memorySize);
	this->registers = new struct registers ();

	memset (this->memory->memory, 0, memorySize);
	memset (this->registers, 0, sizeof (struct registers));
	
	for (int i = 0; i < 256; i++)
		this->instructions[i] = &VirtualMachine::unimplemented_instruction;

	this->instructions[nop] = &VirtualMachine::NOP;
	this->instructions[mov] = &VirtualMachine::MOV;
	this->instructions[cmp] = &VirtualMachine::CMP;
	this->instructions[jmp] = &VirtualMachine::JMP;
	this->instructions[je] = &VirtualMachine::JE;
	this->instructions[jne] = &VirtualMachine::JNE;
	this->instructions[jl] = &VirtualMachine::JL;
	this->instructions[call] = &VirtualMachine::CALL;
	this->instructions[calle] = &VirtualMachine::CALLE;
	this->instructions[ret] = &VirtualMachine::RET;
	this->instructions[iret] = &VirtualMachine::IRET;
	this->instructions[sti] = &VirtualMachine::STI;
	this->instructions[cli] = &VirtualMachine::CLI;
	this->instructions[inc] = &VirtualMachine::INC;
	this->instructions[add] = &VirtualMachine::ADD;
	this->instructions[mul] = &VirtualMachine::MUL;
	this->instructions[or] = &VirtualMachine::OR;
	this->instructions[xor] = &VirtualMachine::XOR;
	this->instructions[shl] = &VirtualMachine::SHL;
	this->instructions[inb] = &VirtualMachine::INB;
	this->instructions[outb] = &VirtualMachine::OUTB;
	this->instructions[ldidt] = &VirtualMachine::LDIDT;
	this->instructions[hlt] = &VirtualMachine::HLT;
}

VirtualMachine::~VirtualMachine () {
	delete memory;
}

void VirtualMachine::unimplemented_instruction () {
	printf ("Opcode: $%02X not implemented", this->memory->readw(this->registers->PC - 1));
	this->status = Halted | Fault;
}

void VirtualMachine::NOP () {
	printf ("nop");
}

void VirtualMachine::OUTB () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::RegisterImmediate:
		{
			uint8_t reg = this->memory->readw (this->registers->PC++);
			uint8_t port = this->memory->readw (this->registers->PC++);

			printf ("inb %s, $%02X", reg_name (reg), port);

			this->set_reg (reg, this->moutb (port));
			break;
		}
		default:
			printf ("outb %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::INB () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::ImmediateImmediate:
		{
			uint8_t port = this->memory->readw (this->registers->PC++);
			uint8_t val = this->memory->readw (this->registers->PC++);

			printf ("inb $%02X, $%02X", port, val);

			this->minb (port, val);
			break;
		}
		case VirtualMachine::ImmediateRegister:
		{
			uint8_t port = this->memory->readw (this->registers->PC++);
			uint8_t reg = this->memory->readw (this->registers->PC++);
			uint8_t val = this->read_reg (reg);

			printf ("inb $%02X, %s($%02X)", port, this->reg_name (reg), val);

			this->minb (port, val);
			break;
		}
		case VirtualMachine::ImmediateIndirect:
		{
			uint8_t port = this->memory->readw (this->registers->PC++);
			uint32_t addr = this->memory->readq (this->registers->PC);
			this->registers->PC += 4;
			uint8_t val = this->memory->readw (addr);

			printf ("inb $%02x, ($%02x)", port, val);

			this->minb (port, val);
			break;
		}
		default:
			printf ("inb %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::MOV () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::Immediate:
		case VirtualMachine::Register:
		case VirtualMachine::Indirect:
		case VirtualMachine::RIndirect:
			printf ("mov %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::RegisterImmediate:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t reg = this->memory->readw (this->registers->PC++);
			printf ("mov %s, ", reg_name (reg));
			uint32_t val = read_val (sz);

			set_reg (reg, val);

			break;
		}
		case VirtualMachine::RegisterRindirect:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t reg = this->memory->readw (this->registers->PC++);
			printf ("mov %s", reg_name (reg));
			printf (", ");
			uint8_t reg2 = this->memory->readw (this->registers->PC++);
			uint32_t reg_val = read_reg_ind (sz, reg2);
			set_reg (reg, reg_val);
			break;
		}
		case VirtualMachine::RIndirectRegister:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t reg = this->memory->readw (this->registers->PC++);
			uint8_t rval = this->memory->readw (this->registers->PC++);
			printf ("mov ");
			write_reg_ind (sz, reg, rval);
			break;
		}
		case VirtualMachine::RIndirectImmediate:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t reg = this->memory->readw (this->registers->PC++);
			printf ("mov ");
			write_reg_rind_val (sz, reg);

			break;
		}
		case VirtualMachine::RIndirectIndirect:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t reg = this->memory->readw (this->registers->PC++);
			uint32_t addr = this->memory->readq (this->registers->PC);
			this->registers->PC += 4;
			printf ("mov ");
			write_reg_rind_val_ind (sz, reg, addr);

			break;
		}
		case VirtualMachine::IndirectRegister:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint32_t addr = this->memory->readq (this->registers->PC);
			this->registers->PC += 4;
			uint8_t reg = this->memory->readw (this->registers->PC++);

			printf ("mov ");
			write_reg (sz, addr, reg);

			break;
		}
		case VirtualMachine::IndirectImmediate:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint32_t addr = this->memory->readq (this->registers->PC);
			this->registers->PC += 4;
			printf ("mov ");
			write_val (sz, addr);

			break;
		}
		case VirtualMachine::RegisterRegister:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t reg = this->memory->readw (this->registers->PC++);
			uint8_t reg2 = this->memory->readw (this->registers->PC++);
			
			printf ("mov ");
			this->set_reg_reg (sz, reg, reg2);
			break;
		}
		default:
			printf ("mov %s unimplemented", addressing_name(mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::HLT () {
	printf ("hlt");
	this->status |= Halted;
}

void VirtualMachine::CMP () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	set_reg (Flags, 0);

	switch (mode) {
		case VirtualMachine::Immediate:
		case VirtualMachine::Register:
		case VirtualMachine::Indirect:
		case VirtualMachine::RIndirect:
			printf ("cmp %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::RegisterImmediate:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t reg = this->memory->readw (this->registers->PC++);
			uint32_t reg_val = read_reg (reg);
			printf ("cmp %s, ", reg_name (reg));
			uint32_t val = read_val (sz);

			int64_t reg_v = (int64_t) reg_val;
			int64_t val_v = (int64_t) val;

			if (reg_v - val_v == 0)
				this->registers->Flags |= Zero;
			if (reg_v - val_v < 0)
				this->registers->Flags |= Underflow;
			if (reg_v - val_v > UINT32_MAX)
				this->registers->Flags |= Overflow;
			if ((reg_v - val_v) % 2 == 0)
				this->registers->Flags |= Parity;

			break;
		}
		case VirtualMachine::RIndirectImmediate:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t reg = this->memory->readw (this->registers->PC++);
			printf ("cmp ", reg_name (reg));
			uint32_t reg_val = read_reg_ind(sz, reg);
			printf (", ");
			uint32_t val = read_val (sz);

			int64_t reg_v = (int64_t) reg_val;
			int64_t val_v = (int64_t) val;

			if (reg_v - val_v == 0)
				this->registers->Flags |= Zero;
			if (reg_v - val_v < 0)
				this->registers->Flags |= Underflow;
			if (reg_v - val_v > UINT32_MAX)
				this->registers->Flags |= Overflow;
			if ((reg_v - val_v) % 2 == 0)
				this->registers->Flags |= Parity;

			break;
		}
		case VirtualMachine::IndirectImmediate:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			printf ("cmp (");
			uint32_t addr_a = read_val (qword);
			printf ("), ");
			uint32_t b = read_val (sz);
			uint32_t a = this->read_val_n (sz, addr_a);

			int64_t val_a = (int64_t) a;
			int64_t val_b = (int64_t) b;

			if (val_a - val_b == 0)
				this->registers->Flags |= Zero;
			if (val_a - val_b < 0)
				this->registers->Flags |= Underflow;
			if (val_a - val_b > UINT32_MAX)
				this->registers->Flags |= Overflow;
			if ((val_a - val_b) % 2 == 0)
				this->registers->Flags |= Parity;

			break;
		}
		default:
			printf ("cmp %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::JMP () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::Immediate:
		{
			uint32_t val = this->memory->readq (this->registers->PC);
			this->registers->PC = val;
			printf ("jmp $%08X", val);
			break;
		}
		case VirtualMachine::Register:
			printf ("jmp %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::Indirect:
			printf ("jmp %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::RIndirect:
			printf ("jmp %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		default:
			printf ("jmp %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::JE () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::Immediate:
		{
			uint32_t val = this->memory->readq (this->registers->PC);
			if (this->registers->Flags & Zero)
				this->registers->PC = val;
			else
				this->registers->PC += 4;
			printf ("je $%08X", val);
			break;
		}
		case VirtualMachine::Register:
			printf ("je %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::Indirect:
			printf ("je %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::RIndirect:
			printf ("je %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		default:
			printf ("je %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::JL () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::Immediate:
		{
			uint32_t val = this->memory->readq (this->registers->PC);
			if (this->registers->Flags & Underflow)
				this->registers->PC = val;
			else
				this->registers->PC += 4;
			printf ("jl $%08X", val);
			break;
		}
		case VirtualMachine::Register:
			printf ("jl %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::Indirect:
			printf ("jl %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::RIndirect:
			printf ("jl %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		default:
			printf ("jl %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::JNE () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::Immediate:
		{
			uint32_t val = this->memory->readq (this->registers->PC);
			if (!(this->registers->Flags & Zero))
				this->registers->PC = val;
			else
				this->registers->PC += 4;
			printf ("jne $%08X", val);
			break;
		}
		case VirtualMachine::Register:
			printf ("jne %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::Indirect:
			printf ("jne %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::RIndirect:
			printf ("jne %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		default:
			printf ("jne %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::XOR () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::Register:
		{
			uint8_t reg = this->memory->readw (this->registers->PC++);
			printf ("xor %s", reg_name(reg));
			set_reg (reg, 0);
			break;
		}
		case VirtualMachine::RegisterImmediate:
		case VirtualMachine::RegisterRegister:
		case VirtualMachine::RegisterIndirect:
		case VirtualMachine::RegisterRindirect:
		case VirtualMachine::IndirectImmediate:
		case VirtualMachine::IndirectRegister:
		case VirtualMachine::IndirectIndirect:
		case VirtualMachine::IndirectRIndirect:
		case VirtualMachine::RIndirectImmediate:
		case VirtualMachine::RIndirectRegister:
		case VirtualMachine::RIndirectIndirect:
		case VirtualMachine::RIndirectRIndirect:
		case VirtualMachine::Indirect:
		case VirtualMachine::RIndirect:
			printf ("xor %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		default:
			printf ("xor %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::OR () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::RegisterImmediate:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t reg = this->memory->readw (this->registers->PC++);
			uint32_t reg_val = read_reg (reg);
			printf ("or %s, ", reg_name (reg));
			uint32_t val = read_val (sz);

			set_reg (reg, reg_val | val);

			break;
		}
		case VirtualMachine::RegisterRegister:
		case VirtualMachine::RegisterIndirect:
		case VirtualMachine::RegisterRindirect:
		case VirtualMachine::IndirectImmediate:
		case VirtualMachine::IndirectRegister:
		case VirtualMachine::IndirectIndirect:
		case VirtualMachine::IndirectRIndirect:
		case VirtualMachine::RIndirectImmediate:
		case VirtualMachine::RIndirectRegister:
		case VirtualMachine::RIndirectIndirect:
		case VirtualMachine::RIndirectRIndirect:
			printf ("or %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		default:
			printf ("or %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::INC () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::Register:
		{
			uint8_t reg = this->memory->readw (this->registers->PC++);
			uint32_t reg_val = read_reg (reg);
			printf ("inc %s", reg_name (reg));
			set_reg (reg, reg_val + 1);
			break;
		}
		case VirtualMachine::RegisterImmediate:
		case VirtualMachine::RegisterRegister:
		case VirtualMachine::RegisterIndirect:
		case VirtualMachine::RegisterRindirect:
		case VirtualMachine::IndirectImmediate:
		case VirtualMachine::IndirectRegister:
		case VirtualMachine::IndirectIndirect:
		case VirtualMachine::IndirectRIndirect:
		case VirtualMachine::RIndirectImmediate:
		case VirtualMachine::RIndirectRegister:
		case VirtualMachine::RIndirectIndirect:
		case VirtualMachine::RIndirectRIndirect:
		case VirtualMachine::Indirect:
		case VirtualMachine::RIndirect:
			printf ("inc %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		default:
			printf ("inc %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::SHL () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::RegisterImmediate:
		{
			uint8_t reg = this->memory->readw (this->registers->PC++);
			uint32_t reg_val = read_reg (reg);
			printf ("shl %s, ", reg_name (reg));
			uint32_t amm = this->memory->readw (this->registers->PC++);

			set_reg (reg, reg_val << amm);

			break;
		}
		case VirtualMachine::RegisterRegister:
		case VirtualMachine::RegisterIndirect:
		case VirtualMachine::RegisterRindirect:
		case VirtualMachine::IndirectImmediate:
		case VirtualMachine::IndirectRegister:
		case VirtualMachine::IndirectIndirect:
		case VirtualMachine::IndirectRIndirect:
		case VirtualMachine::RIndirectImmediate:
		case VirtualMachine::RIndirectRegister:
		case VirtualMachine::RIndirectIndirect:
		case VirtualMachine::RIndirectRIndirect:
			printf ("shl %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		default:
			printf ("shl %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::CALL () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::ImmediateImmediate:
		case VirtualMachine::ImmediateRegister:
		case VirtualMachine::ImmediateIndirect:
		case VirtualMachine::ImmediateRindirect:
		case VirtualMachine::RegisterImmediate:
		case VirtualMachine::RegisterRegister:
		case VirtualMachine::RegisterIndirect:
		case VirtualMachine::RegisterRindirect:
		case VirtualMachine::IndirectImmediate:
		case VirtualMachine::IndirectRegister:
		case VirtualMachine::IndirectIndirect:
		case VirtualMachine::IndirectRIndirect:
		case VirtualMachine::RIndirectImmediate:
		case VirtualMachine::RIndirectRegister:
		case VirtualMachine::RIndirectIndirect:
		case VirtualMachine::RIndirectRIndirect:
			printf ("call %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::Immediate:
		{
			this->pushq (this->registers->PC + 4);
			uint32_t addr = this->memory->readq (this->registers->PC);
			printf ("call %08X", addr);
			this->registers->PC = addr;
			break;
		}
		default:
			printf ("call %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::CALLE () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::ImmediateImmediate:
		case VirtualMachine::ImmediateRegister:
		case VirtualMachine::ImmediateIndirect:
		case VirtualMachine::ImmediateRindirect:
		case VirtualMachine::RegisterImmediate:
		case VirtualMachine::RegisterRegister:
		case VirtualMachine::RegisterIndirect:
		case VirtualMachine::RegisterRindirect:
		case VirtualMachine::IndirectImmediate:
		case VirtualMachine::IndirectRegister:
		case VirtualMachine::IndirectIndirect:
		case VirtualMachine::IndirectRIndirect:
		case VirtualMachine::RIndirectImmediate:
		case VirtualMachine::RIndirectRegister:
		case VirtualMachine::RIndirectIndirect:
		case VirtualMachine::RIndirectRIndirect:
			printf ("calle %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::Immediate:
		{
			uint32_t addr = this->memory->readq (this->registers->PC);
			printf ("calle %08X", addr);
			if (this->registers->Flags & Zero) {
				this->pushq (this->registers->PC + 4);
				this->registers->PC = addr;
			} else
				this->registers->PC += 4;
			break;
		}
		default:
			printf ("calle %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::MUL () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::RegisterImmediate:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t reg = this->memory->readw (this->registers->PC++);
			uint32_t reg_val = read_reg (reg);
			printf ("mul %s, ", this->reg_name (reg));
			uint32_t val = this->read_val (sz);
			
			this->set_reg (reg, reg_val * val);

			break;
		}
		default:
			printf ("mul %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::ADD () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::RegisterRegister:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t a = this->memory->readw (this->registers->PC++);
			uint32_t a_val = read_reg (a);
			uint8_t b = this->memory->readw (this->registers->PC++);
			uint32_t b_val = read_reg (b);
			printf ("add %s, %s", this->reg_name (a), this->reg_name (b));

			this->set_reg (a, a_val + b_val);

			break;
		}
		case VirtualMachine::RegisterImmediate:
		{
			size sz = (size) this->memory->readw (this->registers->PC++);
			uint8_t a = this->memory->readw (this->registers->PC++);
			uint32_t a_val = read_reg (a);
			printf ("add %s, ", this->reg_name (a));
			uint32_t val = this->read_val (sz);

			this->set_reg (a, a_val + val);

			break;
		}
		default:
			printf ("add %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::RET () {
	this->registers->PC = this->popq ();
	printf ("ret");
}

void VirtualMachine::LDIDT () {
	addressing_mode mode = (addressing_mode) this->memory->readw (this->registers->PC++);

	switch (mode) {
		case VirtualMachine::ImmediateImmediate:
		case VirtualMachine::ImmediateRegister:
		case VirtualMachine::ImmediateIndirect:
		case VirtualMachine::ImmediateRindirect:
		case VirtualMachine::RegisterImmediate:
		case VirtualMachine::RegisterRegister:
		case VirtualMachine::RegisterIndirect:
		case VirtualMachine::RegisterRindirect:
		case VirtualMachine::IndirectImmediate:
		case VirtualMachine::IndirectRegister:
		case VirtualMachine::IndirectIndirect:
		case VirtualMachine::IndirectRIndirect:
		case VirtualMachine::RIndirectImmediate:
		case VirtualMachine::RIndirectRegister:
		case VirtualMachine::RIndirectIndirect:
		case VirtualMachine::RIndirectRIndirect:
			printf ("ldidt %s impossible", addressing_name (mode));
			this->status = Halted | Fault;
			break;
		case VirtualMachine::Immediate:
		{
			uint32_t addr = this->memory->readq (this->registers->PC);
			this->registers->PC += 4;
			printf ("ldidt %08X", addr);
			this->int_descs = (int_desc *) (this->memory->memory + addr);
			break;
		}
		default:
			printf ("ldidt %s unimplemented", addressing_name (mode));
			this->status = Halted | Fault;
			break;
	}
}

void VirtualMachine::IRET () {
	this->status = this->popq ();
	this->registers->PC = this->popq ();
	printf ("iret");
}

void VirtualMachine::PUSHA () {
	this->mpusha ();
}

void VirtualMachine::POPA () {
	this->mpopa ();
}

void VirtualMachine::interrupt (uint8_t line) {
	if (interrupts_enabled) {
		this->inter = true;
		this->int_line = line;
	}
}

void VirtualMachine::Start (char *path) {
	for (Hardware *hw : this->hardware)
		hw->Start ();

	this->status |= On;

	for (int i = 0; i < 16; i++)
		this->memory->writeq (0xA0000 + (i * 4), colors[i]);

	std::ifstream input (path, std::ios::binary);
	std::vector<char> data ((std::istreambuf_iterator<char> (input)),
	(std::istreambuf_iterator<char> ()));

	for (size_t s = 0; s < data.size (); s++)
		this->memory->writew (s, data[s]);

	this->timer = new Timer (1024000);
	this->timer->Tick = [&] (uint64_t delta, uint64_t total) {
		std::lock_guard<std::mutex> (this->lock);
		if (!(this->status & Halted) && this->status & On) {
			printf ("%08X: ", this->registers->PC);
			(this->*instructions[this->memory->readw(this->registers->PC++)]) ();
			printf ("\n");
		}

		if (inter && interrupts_enabled) {
			if (this->status & Halted)
				this->status ^= Halted;
			this->pushq (this->registers->PC);
			this->pushq (this->status);
			this->registers->PC = int_descs[int_line].address;
			this->inter = false;
		}

		if (this->status == Off) {
			for (Hardware *hw : this->hardware)
				hw->Stop ();
			this->status |= Halted;
		}
	};
	this->timer->Start ();
}

void VirtualMachine::pushw (uint8_t val) {
	this->registers->SP--;
	memory->writew (read_reg (SP), val);
}
void VirtualMachine::pushd (uint16_t val) {
	this->registers->SP -= 2;
	memory->writed (read_reg (SP), val);
}
void VirtualMachine::pushq (uint32_t val) {
	this->registers->SP -= 4;
	memory->writeq (read_reg (SP), val);
}

uint8_t VirtualMachine::popw () {
	this->registers->SP++;
	auto temp = memory->readw (read_reg (SP));
	return temp;
}
uint16_t VirtualMachine::popd () {
	this->registers->SP += 2;
	auto temp = memory->readd (read_reg (SP));
	return temp;
}
uint32_t VirtualMachine::popq () {
	auto temp = memory->readq (read_reg (SP));
	this->registers->SP += 4;
	return temp;
}

void VirtualMachine::mpusha () {
	this->pushq (this->registers->A.qword);
	this->pushq (this->registers->B.qword);
	this->pushq (this->registers->C.qword);
	this->pushq (this->registers->D.qword);
	this->pushq (this->registers->E.qword);
	this->pushq (this->registers->F.qword);
	this->pushq (this->registers->W.qword);
	this->pushq (this->registers->X.qword);
	this->pushq (this->registers->Y.qword);
	this->pushq (this->registers->Z.qword);
	this->pushd (this->registers->Flags);
	this->pushd (this->registers->Clocks);
	this->pushq (this->registers->CS);
	this->pushq (this->registers->DS);
	this->pushq (this->registers->SS);
	this->pushq (this->registers->PC);
	this->pushq (this->registers->SP);
	this->pushq (this->registers->BP);
}

void VirtualMachine::mpopa () {
	this->pushq (this->registers->BP);
	this->pushq (this->registers->SP);
	this->pushq (this->registers->PC);
	this->pushq (this->registers->SS);
	this->pushq (this->registers->DS);
	this->pushq (this->registers->CS);
	this->pushd (this->registers->Clocks);
	this->pushd (this->registers->Flags);
	this->pushq (this->registers->Z.qword);
	this->pushq (this->registers->Y.qword);
	this->pushq (this->registers->X.qword);
	this->pushq (this->registers->W.qword);
	this->pushq (this->registers->F.qword);
	this->pushq (this->registers->E.qword);
	this->pushq (this->registers->D.qword);
	this->pushq (this->registers->C.qword);
	this->pushq (this->registers->B.qword);
	this->pushq (this->registers->A.qword);
}

uint32_t VirtualMachine::read_reg (uint8_t reg) {
	registers_enum reg_enum = (registers_enum) reg;

	switch (reg_enum) {
		case VirtualMachine::A:
			return this->registers->A.qword;
		case VirtualMachine::AH:
			return this->registers->A.dword.H;
		case VirtualMachine::AL:
			return this->registers->A.dword.L;
		case VirtualMachine::AHH:
			return this->registers->A.word.HH;
		case VirtualMachine::AHL:
			return this->registers->A.word.HL;
		case VirtualMachine::ALH:
			return this->registers->A.word.LH;
		case VirtualMachine::ALL:
			return this->registers->A.word.LL;
		case VirtualMachine::B:
			return this->registers->B.qword;
		case VirtualMachine::BH:
			return this->registers->B.dword.H;
		case VirtualMachine::BL:
			return this->registers->B.dword.L;
		case VirtualMachine::BHH:
			return this->registers->B.word.HH;
		case VirtualMachine::BHL:
			return this->registers->B.word.HL;
		case VirtualMachine::BLH:
			return this->registers->B.word.LH;
		case VirtualMachine::BLL:
			return this->registers->B.word.LL;
		case VirtualMachine::C:
			return this->registers->C.qword;
		case VirtualMachine::CH:
			return this->registers->C.dword.H;
		case VirtualMachine::CL:
			return this->registers->C.dword.L;
		case VirtualMachine::CHH:
			return this->registers->C.word.HH;
		case VirtualMachine::CHL:
			return this->registers->C.word.HL;
		case VirtualMachine::CLH:
			return this->registers->C.word.LH;
		case VirtualMachine::CLL:
			return this->registers->C.word.LL;
		case VirtualMachine::D:
			return this->registers->D.qword;
		case VirtualMachine::DH:
			return this->registers->D.dword.H;
		case VirtualMachine::DL:
			return this->registers->D.dword.L;
		case VirtualMachine::DHH:
			return this->registers->D.word.HH;
		case VirtualMachine::DHL:
			return this->registers->D.word.HL;
		case VirtualMachine::DLH:
			return this->registers->D.word.LH;
		case VirtualMachine::DLL:
			return this->registers->D.word.LL;
		case VirtualMachine::E:
			return this->registers->E.qword;
		case VirtualMachine::EH:
			return this->registers->E.dword.H;
		case VirtualMachine::EL:
			return this->registers->E.dword.L;
		case VirtualMachine::EHH:
			return this->registers->E.word.HH;
		case VirtualMachine::EHL:
			return this->registers->E.word.HL;
		case VirtualMachine::ELH:
			return this->registers->E.word.LH;
		case VirtualMachine::ELL:
			return this->registers->E.word.LL;
		case VirtualMachine::F:
			return this->registers->F.qword;
		case VirtualMachine::FH:
			return this->registers->F.dword.H;
		case VirtualMachine::FL:
			return this->registers->F.dword.L;
		case VirtualMachine::FHH:
			return this->registers->F.word.HH;
		case VirtualMachine::FHL:
			return this->registers->F.word.HL;
		case VirtualMachine::FLH:
			return this->registers->F.word.LH;
		case VirtualMachine::FLL:
			return this->registers->F.word.LL;
		case VirtualMachine::W:
			return this->registers->W.qword;
		case VirtualMachine::WH:
			return this->registers->W.dword.H;
		case VirtualMachine::WL:
			return this->registers->W.dword.L;
		case VirtualMachine::WHH:
			return this->registers->W.word.HH;
		case VirtualMachine::WHL:
			return this->registers->W.word.HL;
		case VirtualMachine::WLH:
			return this->registers->W.word.LH;
		case VirtualMachine::WLL:
			return this->registers->W.word.LL;
		case VirtualMachine::X:
			return this->registers->X.qword;
		case VirtualMachine::XH:
			return this->registers->X.dword.H;
		case VirtualMachine::XL:
			return this->registers->X.dword.L;
		case VirtualMachine::XHH:
			return this->registers->X.word.HH;
		case VirtualMachine::XHL:
			return this->registers->X.word.HL;
		case VirtualMachine::XLH:
			return this->registers->X.word.LH;
		case VirtualMachine::XLL:
			return this->registers->X.word.LL;
		case VirtualMachine::Y:
			return this->registers->Y.qword;
		case VirtualMachine::YH:
			return this->registers->Y.dword.H;
		case VirtualMachine::YL:
			return this->registers->Y.dword.L;
		case VirtualMachine::YHH:
			return this->registers->Y.word.HH;
		case VirtualMachine::YHL:
			return this->registers->Y.word.HL;
		case VirtualMachine::YLH:
			return this->registers->Y.word.LH;
		case VirtualMachine::YLL:
			return this->registers->Y.word.LL;
		case VirtualMachine::Z:
			return this->registers->Z.qword;
		case VirtualMachine::ZH:
			return this->registers->Z.dword.H;
		case VirtualMachine::ZL:
			return this->registers->Z.dword.L;
		case VirtualMachine::ZHH:
			return this->registers->Z.word.HH;
		case VirtualMachine::ZHL:
			return this->registers->Z.word.HL;
		case VirtualMachine::ZLH:
			return this->registers->Z.word.LH;
		case VirtualMachine::ZLL:
			return this->registers->Z.word.LL;
		case VirtualMachine::Flags:
			return this->registers->Flags;
		case VirtualMachine::Clocks:
			return this->registers->Clocks;
		case VirtualMachine::CS:
			return this->registers->CS;
		case VirtualMachine::DS:
			return this->registers->DS;
		case VirtualMachine::SS:
			return this->registers->SS;
		case VirtualMachine::PC:
			return this->registers->PC;
		case VirtualMachine::SP:
			return this->registers->SP;
		case VirtualMachine::BP:
			return this->registers->BP;
	}
}

void VirtualMachine::set_reg (uint8_t reg, uint32_t val) {
	registers_enum renum = (registers_enum) reg;

	switch (renum) {
		case VirtualMachine::A:
			this->registers->A.qword = val;
			break;
		case VirtualMachine::AH:
			this->registers->A.dword.H = val;
			break;
		case VirtualMachine::AL:
			this->registers->A.dword.L = val;
			break;
		case VirtualMachine::AHH:
			this->registers->A.word.HH = val;
			break;
		case VirtualMachine::AHL:
			this->registers->A.word.HL = val;
			break;
		case VirtualMachine::ALH:
			this->registers->A.word.LH = val;
			break;
		case VirtualMachine::ALL:
			this->registers->A.word.LL = val;
			break;
		case VirtualMachine::B:
			this->registers->B.qword = val;
			break;
		case VirtualMachine::BH:
			this->registers->B.dword.H = val;
			break;
		case VirtualMachine::BL:
			this->registers->B.dword.L = val;
			break;
		case VirtualMachine::BHH:
			this->registers->B.word.HH = val;
			break;
		case VirtualMachine::BHL:
			this->registers->B.word.HL = val;
			break;
		case VirtualMachine::BLH:
			this->registers->B.word.LH = val;
			break;
		case VirtualMachine::BLL:
			this->registers->B.word.LL = val;
			break;
		case VirtualMachine::C:
			this->registers->C.qword = val;
			break;
		case VirtualMachine::CH:
			this->registers->C.dword.H = val;
			break;
		case VirtualMachine::CL:
			this->registers->C.dword.L = val;
			break;
		case VirtualMachine::CHH:
			this->registers->C.word.HH = val;
			break;
		case VirtualMachine::CHL:
			this->registers->C.word.HL = val;
			break;
		case VirtualMachine::CLH:
			this->registers->C.word.LH = val;
			break;
		case VirtualMachine::CLL:
			this->registers->C.word.LL = val;
			break;
		case VirtualMachine::D:
			this->registers->D.qword = val;
			break;
		case VirtualMachine::DH:
			this->registers->D.dword.H = val;
			break;
		case VirtualMachine::DL:
			this->registers->D.dword.L = val;
			break;
		case VirtualMachine::DHH:
			this->registers->D.word.HH = val;
			break;
		case VirtualMachine::DHL:
			this->registers->D.word.HL = val;
			break;
		case VirtualMachine::DLH:
			this->registers->D.word.LH = val;
			break;
		case VirtualMachine::DLL:
			this->registers->D.word.LL = val;
			break;
		case VirtualMachine::E:
			this->registers->E.qword = val;
			break;
		case VirtualMachine::EH:
			this->registers->E.dword.H = val;
			break;
		case VirtualMachine::EL:
			this->registers->E.dword.L = val;
			break;
		case VirtualMachine::EHH:
			this->registers->E.word.HH = val;
			break;
		case VirtualMachine::EHL:
			this->registers->E.word.HL = val;
			break;
		case VirtualMachine::ELH:
			this->registers->E.word.LH = val;
			break;
		case VirtualMachine::ELL:
			this->registers->E.word.LL = val;
			break;
		case VirtualMachine::F:
			this->registers->F.qword = val;
			break;
		case VirtualMachine::FH:
			this->registers->F.dword.H = val;
			break;
		case VirtualMachine::FL:
			this->registers->F.dword.L = val;
			break;
		case VirtualMachine::FHH:
			this->registers->F.word.HH = val;
			break;
		case VirtualMachine::FHL:
			this->registers->F.word.HL = val;
			break;
		case VirtualMachine::FLH:
			this->registers->F.word.LH = val;
			break;
		case VirtualMachine::FLL:
			this->registers->F.word.LL = val;
			break;
		case VirtualMachine::W:
			this->registers->W.qword = val;
			break;
		case VirtualMachine::WH:
			this->registers->W.dword.H = val;
			break;
		case VirtualMachine::WL:
			this->registers->W.dword.L = val;
			break;
		case VirtualMachine::WHH:
			this->registers->W.word.HH = val;
			break;
		case VirtualMachine::WHL:
			this->registers->W.word.HL = val;
			break;
		case VirtualMachine::WLH:
			this->registers->W.word.LH = val;
			break;
		case VirtualMachine::WLL:
			this->registers->W.word.LL = val;
			break;
		case VirtualMachine::X:
			this->registers->X.qword = val;
			break;
		case VirtualMachine::XH:
			this->registers->X.dword.H = val;
			break;
		case VirtualMachine::XL:
			this->registers->X.dword.L = val;
			break;
		case VirtualMachine::XHH:
			this->registers->X.word.HH = val;
			break;
		case VirtualMachine::XHL:
			this->registers->X.word.HL = val;
			break;
		case VirtualMachine::XLH:
			this->registers->X.word.LH = val;
			break;
		case VirtualMachine::XLL:
			this->registers->X.word.LL = val;
			break;
		case VirtualMachine::Y:
			this->registers->Y.qword = val;
			break;
		case VirtualMachine::YH:
			this->registers->Y.dword.H = val;
			break;
		case VirtualMachine::YL:
			this->registers->Y.dword.L = val;
			break;
		case VirtualMachine::YHH:
			this->registers->Y.word.HH = val;
			break;
		case VirtualMachine::YHL:
			this->registers->Y.word.HL = val;
			break;
		case VirtualMachine::YLH:
			this->registers->Y.word.LH = val;
			break;
		case VirtualMachine::YLL:
			this->registers->Y.word.LL = val;
			break;
		case VirtualMachine::Z:
			this->registers->Z.qword = val;
			break;
		case VirtualMachine::ZH:
			this->registers->Z.dword.H = val;
			break;
		case VirtualMachine::ZL:
			this->registers->Z.dword.L = val;
			break;
		case VirtualMachine::ZHH:
			this->registers->Z.word.HH = val;
			break;
		case VirtualMachine::ZHL:
			this->registers->Z.word.HL = val;
			break;
		case VirtualMachine::ZLH:
			this->registers->Z.word.LH = val;
			break;
		case VirtualMachine::ZLL:
			this->registers->Z.word.LL = val;
			break;
		case VirtualMachine::Flags:
			this->registers->Flags = val;
			break;
		case VirtualMachine::Clocks:
			this->registers->Clocks = val;
			break;
		case VirtualMachine::CS:
			this->registers->CS = val;
			break;
		case VirtualMachine::DS:
			this->registers->DS = val;
			break;
		case VirtualMachine::SS:
			this->registers->SS = val;
			break;
		case VirtualMachine::PC:
			this->registers->PC = val;
			break;
		case VirtualMachine::SP:
			this->registers->SP = val;
			break;
		case VirtualMachine::BP:
			this->registers->BP = val;
			break;
	}
}

const char *VirtualMachine::reg_name (uint8_t reg) {
	registers_enum rege = (registers_enum) reg;

	switch (rege) {
		case VirtualMachine::A:
			return "A";
		case VirtualMachine::AH:
			return "AH";
		case VirtualMachine::AL:
			return "AL";
		case VirtualMachine::AHH:
			return "AHH";
		case VirtualMachine::AHL:
			return "AHL";
		case VirtualMachine::ALH:
			return "ALH";
		case VirtualMachine::ALL:
			return "ALL";
		case VirtualMachine::B:
			return "B";
		case VirtualMachine::BH:
			return "BH";
		case VirtualMachine::BL:
			return "BL";
		case VirtualMachine::BHH:
			return "BHH";
		case VirtualMachine::BHL:
			return "BHL";
		case VirtualMachine::BLH:
			return "BLH";
		case VirtualMachine::BLL:
			return "BLL";
		case VirtualMachine::C:
			return "C";
		case VirtualMachine::CH:
			return "CH";
		case VirtualMachine::CL:
			return "CL";
		case VirtualMachine::CHH:
			return "CHH";
		case VirtualMachine::CHL:
			return "CHL";
		case VirtualMachine::CLH:
			return "CLH";
		case VirtualMachine::CLL:
			return "CLL";
		case VirtualMachine::D:
			return "D";
		case VirtualMachine::DH:
			return "DH";
		case VirtualMachine::DL:
			return "DL";
		case VirtualMachine::DHH:
			return "DHH";
		case VirtualMachine::DHL:
			return "DHL";
		case VirtualMachine::DLH:
			return "DLH";
		case VirtualMachine::DLL:
			return "DLL";
		case VirtualMachine::E:
			return "E";
		case VirtualMachine::EH:
			return "EH";
		case VirtualMachine::EL:
			return "EL";
		case VirtualMachine::EHH:
			return "EHH";
		case VirtualMachine::EHL:
			return "EHL";
		case VirtualMachine::ELH:
			return "ELH";
		case VirtualMachine::ELL:
			return "ELL";
		case VirtualMachine::F:
			return "F";
		case VirtualMachine::FH:
			return "FH";
		case VirtualMachine::FL:
			return "FL";
		case VirtualMachine::FHH:
			return "FHH";
		case VirtualMachine::FHL:
			return "FHL";
		case VirtualMachine::FLH:
			return "FLH";
		case VirtualMachine::FLL:
			return "FLL";
		case VirtualMachine::W:
			return "W";
		case VirtualMachine::WH:
			return "WH";
		case VirtualMachine::WL:
			return "WL";
		case VirtualMachine::WHH:
			return "WHH";
		case VirtualMachine::WHL:
			return "WHL";
		case VirtualMachine::WLH:
			return "WLH";
		case VirtualMachine::WLL:
			return "WLL";
		case VirtualMachine::X:
			return "X";
		case VirtualMachine::XH:
			return "XH";
		case VirtualMachine::XL:
			return "XL";
		case VirtualMachine::XHH:
			return "XHH";
		case VirtualMachine::XHL:
			return "XHL";
		case VirtualMachine::XLH:
			return "XLH";
		case VirtualMachine::XLL:
			return "XLL";
		case VirtualMachine::Y:
			return "Y";
		case VirtualMachine::YH:
			return "YH";
		case VirtualMachine::YL:
			return "YL";
		case VirtualMachine::YHH:
			return "YHH";
		case VirtualMachine::YHL:
			return "YHL";
		case VirtualMachine::YLH:
			return "YLH";
		case VirtualMachine::YLL:
			return "YLL";
		case VirtualMachine::Z:
			return "Z";
		case VirtualMachine::ZH:
			return "ZH";
		case VirtualMachine::ZL:
			return "ZL";
		case VirtualMachine::ZHH:
			return "ZHH";
		case VirtualMachine::ZHL:
			return "ZHL";
		case VirtualMachine::ZLH:
			return "ZLH";
		case VirtualMachine::ZLL:
			return "ZLL";
		case VirtualMachine::Flags:
			return "Flags";
		case VirtualMachine::Clocks:
			return "Clocks";
		case VirtualMachine::CS:
			return "CS";
		case VirtualMachine::DS:
			return "DS";
		case VirtualMachine::SS:
			return "SS";
		case VirtualMachine::PC:
			return "PC";
		case VirtualMachine::SP:
			return "SP";
		case VirtualMachine::BP:
			return "BP";
	}
}

const char *VirtualMachine::addressing_name (addressing_mode mode) {
	switch (mode) {
		case VirtualMachine::ImmediateImmediate:
			return "Immediate Immediate";
		case VirtualMachine::ImmediateRegister:
			return "Immediate Register";
		case VirtualMachine::ImmediateIndirect:
			return "Immediate Indirect";
		case VirtualMachine::ImmediateRindirect:
			return "Immediate RegisterIndirect";
		case VirtualMachine::RegisterImmediate:
			return "Register Immediate";
		case VirtualMachine::RegisterRegister:
			return "Register Register";
		case VirtualMachine::RegisterIndirect:
			return "Register Indirect";
		case VirtualMachine::RegisterRindirect:
			return "Register RegisterIndirect";
		case VirtualMachine::IndirectImmediate:
			return "Indirect Immediate";
		case VirtualMachine::IndirectRegister:
			return "Indirect Register";
		case VirtualMachine::IndirectIndirect:
			return "Indirect Indirect";
		case VirtualMachine::IndirectRIndirect:
			return "Indirect RegisterIndirect";
		case VirtualMachine::RIndirectImmediate:
			return "RegisterIndirect Immediate";
		case VirtualMachine::RIndirectRegister:
			return "RegisterIndirect Register";
		case VirtualMachine::RIndirectIndirect:
			return "RegisterIndirect Indirect";
		case VirtualMachine::RIndirectRIndirect:
			return "RegisterIndirect RegisterIndirect";
		case VirtualMachine::Immediate:
			return "Immediate";
		case VirtualMachine::Register:
			return "Register";
		case VirtualMachine::Indirect:
			return "Indirect";
		case VirtualMachine::RIndirect:
			return "RegisterIndirect";
	}
}

uint32_t VirtualMachine::read_val_n (size sz, uint32_t addr) {
	uint32_t val;
	switch (sz) {
		case VirtualMachine::word:
			val = this->memory->readw (addr);
			break;
		case VirtualMachine::dword:
			val = this->memory->readd (addr);
			break;
		case VirtualMachine::qword:
			val = this->memory->readq (addr);
			break;
	}
	return val;
}

uint32_t VirtualMachine::read_val (size sz) {
	uint32_t val;
	switch (sz) {
		case VirtualMachine::word:
			val = this->memory->readw (this->registers->PC++);
			printf ("$%02X", val);
			break;
		case VirtualMachine::dword:
			val = this->memory->readd (this->registers->PC);
			this->registers->PC += 2;
			printf ("$%04X", val);
			break;
		case VirtualMachine::qword:
			val = this->memory->readq (this->registers->PC);
			this->registers->PC += 4;
			printf ("$%08X", val);
			break;
	}
	return val;
}

void VirtualMachine::write_val (size sz, uint32_t addr) {
	uint32_t val;
	switch (sz) {
		case VirtualMachine::word:
			printf ("($%08X), ", addr);
			val = read_val (sz);
			memory->writew (addr, val);
			break;
		case VirtualMachine::dword:
			printf ("($%08X), ", addr);
			val = read_val (sz);
			memory->writed (addr, val);
			break;
		case VirtualMachine::qword:
			printf ("($%08X), ", addr);
			val = read_val (sz);
			memory->writeq (addr, val);
			break;
	}
}

void VirtualMachine::write_reg (size sz, uint32_t addr, uint8_t reg) {
	switch (sz) {
		case VirtualMachine::word:
		{
			uint8_t val = read_reg (reg);
			printf ("($%08X), %s", addr, reg_name (reg));
			memory->writew (addr, val);
			break;
		}
		case VirtualMachine::dword:
		{
			uint16_t val = read_reg (reg);
			printf ("($%08X), %s", addr, reg_name (reg));
			memory->writed (addr, val);
			break;
		}
		case VirtualMachine::qword:
		{
			uint32_t val = read_reg (reg);
			printf ("($%08X), %s", addr, reg_name (reg));
			memory->writeq (addr, val);
			break;
		}
	}
}

void VirtualMachine::write_reg_ind (size sz, uint8_t reg, uint8_t reg2) {
	uint32_t addr = read_reg (reg);
	
	switch (sz) {
		case VirtualMachine::word:
		{
			uint8_t val = read_reg (reg2);
			printf ("(%s), %s", reg_name (reg), reg_name (reg2));
			memory->writew (addr, val);
			break;
		}
		case VirtualMachine::dword:
		{
			uint16_t val = read_reg (reg2);
			printf ("(%s), %s", reg_name (reg), reg_name (reg2));
			memory->writed (addr, val);
			break;
		}
		case VirtualMachine::qword:
		{
			uint32_t val = read_reg (reg2);
			printf ("(%s), %s", reg_name (reg), reg_name (reg2));
			memory->writeq (addr, val);
			break;
		}
	}
}

void VirtualMachine::set_reg_reg (size sz, uint8_t a, uint8_t b) {
	switch (sz) {
		case VirtualMachine::word:
		{
			uint32_t aval = read_reg (a);
			uint8_t bval = read_reg (b);
			printf ("%s, %s", reg_name (a), reg_name (b));
			this->set_reg (a, (aval & 0xFFFFFF00) | bval);
			break;
		}
		case VirtualMachine::dword:
		{
			uint32_t aval = read_reg (a);
			uint16_t bval = read_reg (b);
			printf ("%s, %s", reg_name (a), reg_name (b));
			this->set_reg (a, (aval & 0xFFFF0000) | bval);
			break;
		}
		case VirtualMachine::qword:
		{
			uint32_t aval = read_reg (a);
			uint16_t bval = read_reg (b);
			printf ("%s, %s", reg_name (a), reg_name (b));
			this->set_reg (a, bval);
			break;
		}
	}
}

void VirtualMachine::write_reg_rind_val (size sz, uint8_t reg) {
	uint32_t addr = read_reg (reg);

	switch (sz) {
		case VirtualMachine::word:
		{
			printf ("(%s), ", reg_name (reg));
			memory->writew (addr, read_val (sz));
			break;
		}
		case VirtualMachine::dword:
		{
			printf ("(%s), ", reg_name (reg));
			memory->writed (addr, read_val (sz));
			break;
		}
		case VirtualMachine::qword:
		{
			printf ("(%s), ", reg_name (reg));
			memory->writeq (addr, read_val (sz));
			break;
		}
	}
}

void VirtualMachine::write_reg_rind_val_ind (size sz, uint8_t reg, uint32_t vaddr) {
	uint32_t addr = read_reg (reg);

	switch (sz) {
		case VirtualMachine::word:
		{
			printf ("(%s), ($%02X)", reg_name (reg), vaddr);
			memory->writew (addr, memory->readw (vaddr));
			break;
		}
		case VirtualMachine::dword:
		{
			printf ("(%s), ($%04X)", reg_name (reg), vaddr);
			memory->writed (addr, memory->readd (vaddr));
			break;
		}
		case VirtualMachine::qword:
		{
			printf ("(%s), ($%08X)", reg_name (reg), vaddr);
			memory->writeq (addr, memory->readq (vaddr));
			break;
		}
	}
}

uint32_t VirtualMachine::read_reg_ind (size sz, uint8_t reg) {
	uint32_t val;
	switch (sz) {
		case VirtualMachine::word:
			val = memory->readw(read_reg (reg));
			printf ("(%s)", reg_name (reg));
			break;
		case VirtualMachine::dword:
			val = memory->readd (read_reg (reg));
			printf ("(%s)", reg_name (reg));
			break;
		case VirtualMachine::qword:
			val = memory->readq (read_reg (reg));
			printf ("(%s)", reg_name (reg));
			break;
	}
	return val;
}

void VirtualMachine::AddHardware (Hardware *hardware) {
	this->hardware.push_back (hardware);
}
void VirtualMachine::AddMemoryRegion (MemoryRegion *region) {
	this->memory->AddMemoryRegion (region);
}

void VirtualMachine::minb (uint8_t port, uint8_t value) {
	if (has_in (port, b))
		inp (port, b, value);
}
void VirtualMachine::mind (uint8_t port, uint16_t value) {
	if (has_in (port, d))
		inp (port, d, value);
}
void VirtualMachine::minq (uint8_t port, uint32_t value) {
	if (has_in (port, q))
		inp (port, q, value);
}

uint8_t VirtualMachine::moutb (uint8_t port) {
	if (has_out (port, b))
		return outp (port, b);
	return 0;
}
uint16_t VirtualMachine::moutd (uint8_t port) {
	if (has_out (port, d))
		return outp (port, d);
	return 0;
}
uint32_t VirtualMachine::moutq (uint8_t port) {
	if (has_out (port, q))
		return outp (port, q);
	return 0;
}
