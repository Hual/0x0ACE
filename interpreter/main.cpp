#include <cstdio>
#include <cstdint>
#include <vector>
#include <stack>
#include <functional>

#define ZFLAG(loc, exp) loc.zf=((exp)==0)

enum e_opcode : uint8_t
{
	mov = 0x00,
	or = 0x01,
	xor = 0x02,
	and = 0x03,
	not = 0x04,
	add = 0x05,
	sub = 0x06,
	mul = 0x07,
	shl = 0x08,
	shr = 0x09,
	inc = 0x0a,
	dec = 0x0b,
	push = 0x0c,
	pop = 0x0d,
	cmp = 0x0e,
	jnz = 0x0f,
	jz  = 0x10
};

enum e_reg : uint8_t
{
	r0 = 0,
	r1 = 1,
	r2 = 2,
	r3 = 3
};

enum e_mod : uint8_t
{
	imm = 0,
	reg = 1,
	reg_imm = 2,
	reg_reg = 3
};

char* disasm_opcode[]
{
	"mov ",
	"or  ",
	"xor ",
	"and ",
	"not ",
	"add ",
	"sub ",
	"mul ",
	"shl ",
	"shr ",
	"inc ",
	"dec ",
	"push",
	"pop ",
	"cmp ",
	"jnz ",
	"jz  "
};

char* disasm_mod[0x04]
{
	"imm    ",
	"reg    ",
	"reg-imm",
	"reg-reg"
};

char* disasm_reg[0x04]
{
	"r0",
	"r1",
	"r2",
	"r3"
};

struct instruction_minimal
{
	e_opcode code : 8;
	e_mod mod : 4;
	e_reg dst_reg : 2;
	e_reg src_reg : 2;
};

struct instruction
{
	typedef std::vector<instruction> list;

	instruction_minimal base;
	uint16_t imm = 0;

	instruction(uint16_t* bytes) : base(*reinterpret_cast<instruction_minimal*>(bytes)) { }
};

static struct {
	std::stack<uint16_t> stack;
	bool zf;
	uint16_t pc;
	uint16_t registers[4];
} cpu;

std::function<void(uint16_t* dst, const uint16_t value)> ops[0x11] =
{
	[](auto dst, auto value) { *dst = value; },								// mov
	[](auto dst, auto value) { ZFLAG(cpu, *dst |= value); },				// or
	[](auto dst, auto value) { ZFLAG(cpu, *dst ^= value); },				// xor
	[](auto dst, auto value) { ZFLAG(cpu, *dst &= value); },				// and
	[](auto dst, auto value) { ZFLAG(cpu, *dst = ~*dst); },					// neg
	[](auto dst, auto value) { ZFLAG(cpu, *dst += value); },				// add
	[](auto dst, auto value) { ZFLAG(cpu, *dst -= value); },				// sub
	[](auto dst, auto value) { ZFLAG(cpu, *dst *= value); },				// mul
	[](auto dst, auto value) { ZFLAG(cpu, *dst <<= value); },				// shl
	[](auto dst, auto value) { ZFLAG(cpu, *dst >>= value); },				// shr
	[](auto dst, auto value) { ZFLAG(cpu, ++*dst); },						// inc
	[](auto dst, auto value) { ZFLAG(cpu, --*dst); },						// dec
	[](auto dst, auto value) { cpu.stack.push(value); },					// push
	[](auto dst, auto value) { *dst = cpu.stack.top(); cpu.stack.pop(); },	// pop
	[](auto dst, auto value) { ZFLAG(cpu, *dst - value); },					// cmp
	[](auto dst, auto value) { if (!cpu.zf) cpu.pc = value; },				// jnz
	[](auto dst, auto value) { if (cpu.zf) cpu.pc = value; }				// jz
};

bool load(const char* path, instruction::list& output)
{
	FILE* file = ::fopen(path, "rb");
	uint16_t buf;
	bool new_instruction = true;

	if (!file)
		return false;

	while (::fread(&buf, sizeof(uint16_t), 1, file) == 1) {
		size_t ipc = output.size() - 1;

		if (new_instruction) {
			output.push_back(&buf);
			++ipc;

			if (output[ipc].base.mod == reg_imm || output[ipc].base.mod == imm) {
				new_instruction = false;
			}
		}
		else {
			output[ipc].imm = buf;
			new_instruction = true;
		}
	}

	::fclose(file);

	return new_instruction == true; // if reached EOF before new instruction then instruction is incomplete and code is corrupted
}

void trace(const instruction& instr)
{
	switch (instr.base.mod) {
	case imm:
		::printf("%s %x\n", disasm_opcode[instr.base.code], instr.imm);
		break;
	case reg:
		::printf("%s %s\n", disasm_opcode[instr.base.code], disasm_reg[instr.base.dst_reg]);
		break;
	case reg_imm:
		::printf("%s %s, %x\n", disasm_opcode[instr.base.code], disasm_reg[instr.base.dst_reg], instr.imm);
		break;
	case reg_reg:
		::printf("%s %s, %s\n", disasm_opcode[instr.base.code], disasm_reg[instr.base.dst_reg], disasm_reg[instr.base.src_reg]);
		break;
	}
}

void disasm(const instruction::list& bytecode)
{
	::puts("Static disassembly: \n");

	for (size_t i = 0; i < bytecode.size(); ++i) {
		switch (bytecode[i].base.mod) {
		case imm:
			::printf("[%02x] %s %x\n", i, disasm_opcode[bytecode[i].base.code], bytecode[i].imm);
			break;
		case reg:
			::printf("[%02x] %s %s\n", i, disasm_opcode[bytecode[i].base.code], disasm_reg[bytecode[i].base.dst_reg]);
			break;
		case reg_imm:
			::printf("[%02x] %s %s, %x\n", i, disasm_opcode[bytecode[i].base.code], disasm_reg[bytecode[i].base.dst_reg], bytecode[i].imm);
			break;
		case reg_reg:
			::printf("[%02x] %s %s, %s\n", i, disasm_opcode[bytecode[i].base.code], disasm_reg[bytecode[i].base.dst_reg], disasm_reg[bytecode[i].base.src_reg]);
			break;
		}
	}

	::puts("\n");
}

static void exec(const instruction::list& bytecode)
{
	cpu.pc = cpu.zf = 0;
	cpu.registers[0] = cpu.registers[1] = cpu.registers[2] = cpu.registers[3] = 0;
	cpu.stack = std::stack<uint16_t>();

	while (cpu.pc != bytecode.size()) {
		uint16_t* ptr = nullptr;
		uint16_t value = 0;
		const size_t pc_current = cpu.pc++;

		if (bytecode[pc_current].base.mod == imm) {
			value = bytecode[pc_current].imm;
		}
		else if (bytecode[pc_current].base.mod == reg) {
			ptr = &cpu.registers[bytecode[pc_current].base.dst_reg];
		}
		else if (bytecode[pc_current].base.mod == reg_imm) {
			ptr = &cpu.registers[bytecode[pc_current].base.dst_reg];
			value = bytecode[pc_current].imm;
		}
		else if (bytecode[pc_current].base.mod == reg_reg) {
			ptr = &cpu.registers[bytecode[pc_current].base.dst_reg];
			value = cpu.registers[bytecode[pc_current].base.src_reg];
		}

#ifdef _DEBUG
		trace(bytecode[pc_current]);
#endif
		
		ops[bytecode[pc_current].base.code](ptr, value);
	}

	::printf("reg0=%04x&reg1=%04x&reg2=%04x&reg3=%04x", cpu.registers[0], cpu.registers[1], cpu.registers[2], cpu.registers[3]);
}

int main(int argc, char* argv[])
{
	instruction::list bytecode;

	static_assert(sizeof(instruction) == sizeof(uint16_t) * 2, "Size of instruction isn't 32 bits!");

	if (argc < 2) {
		::fprintf(stderr, "No file specified.");
	}
	else if (load(argv[1], bytecode)) {

#ifdef _DEBUG
		disasm(bytecode);
#endif

		exec(bytecode);
	}
	else {
		::fprintf(stderr, "Assembly is corrupted.");
	}

	return 0;
}
