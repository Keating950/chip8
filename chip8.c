#include "chip8.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#define ERROR_EXIT(msg)                                                   \
	do {                                                                  \
		perror(msg);                                                      \
		exit(EXIT_FAILURE);                                               \
	} while (0)

#define ROWS 0x40
#define COLS 0x20
#define ON 0xFFFFFFFF
#define OFF 0

extern uint8_t await_keypress(void);

chip8_vm init_chip8()
{
	srand(clock()); // for opcode requiring RNG
	chip8_vm vm = {
		.mem = { 0 },
		.v = { 0 },
		.idx = 0,
		.pc = 200,
		.call_stack = { 0 },
		.sp = 0,
		.screen = { { OFF } },
		.delay_timer = 0,
		.sound_timer = 0,
		.keyboard = { false },
		.draw_flag = false,
	};
	return vm;
}

void load_rom(const char *path, chip8_vm *vm)
{
	size_t rom_size;
	size_t bytes_read;
	FILE *f = fopen(path, "rb");
	if (!f)
		ERROR_EXIT("Failed to open file");
	fseek(f, 0L, SEEK_END);
	rom_size = ftell(f);
	if (rom_size > 0x800)
		ERROR_EXIT("Rom is too large");
	rewind(f);
	bytes_read = fread((vm->mem + 0x200), sizeof(uint8_t), rom_size, f);
	if (bytes_read != rom_size)
		ERROR_EXIT("Error reading file");
}

void print_rom(chip8_vm vm)
{
	do {
		for (int i = 0; i < 3; i++) {
			uint16_t opcode = vm.mem[vm.pc] << 8u | vm.mem[vm.pc + 1];
			printf("%#X  ", opcode);
			vm.pc += 2;
		}
		puts("\n");
	} while (vm.pc < 800);
}

static void draw_sprite(chip8_vm *vm, int x, int y, int height)
{
}

void vm_cycle(chip8_vm *vm)
{
	const unsigned short opcode =
		vm->mem[vm->pc] << 8 | vm->mem[vm->pc + 1];
	fprintf(stderr, "%04X\n", opcode);
	static const void *opcode_handles[] = {
		&&zero_ops,	  &&jump,		 &&jump_and_link, &&reg_eq_im,
		&&reg_neq_im, &&reg_eq_reg,	 &&load_halfword, &&add_halfword,
		&&math,		  &&reg_neq_reg, &&set_idx,		  &&jump_idx_plus_reg,
		&&random_and, &&draw,		 &&exxx_keyops,	  &&fxxx_ops,
	};

	if (opcode_handles[(opcode & 0xF000u) >> 12])
		goto *opcode_handles[(opcode & 0xF000u) >> 12];
	else {
		fprintf(stderr, "Error: Unknown opcode %#X\n", opcode);
		exit(EXIT_FAILURE);
	}

zero_ops:
	switch (opcode & 0x00FFu) {
	case 0xE0:
		// clear screen
		vm->draw_flag = true;
		for (int i = 0; i < 0x20; i++)
			memset(vm->screen + i, OFF, 0x40);
		vm->pc += 2;
		return;
	case 0xEE:
		// jr $ra
		if (vm->sp > 0)
			vm->pc = vm->call_stack[--vm->sp] + 2;
		else
			ERROR_EXIT("Attempted pop from empty VM Stack\n");
		return;
	default:
		// 0NNN: deprecated instruction
		vm->pc += 2;
		return;
	}
jump:
	// 1NNN: jump (don't store pc)
	vm->pc = opcode & 0x0FFFu;
	return;
jump_and_link:
	// 2NNN: call subroutine (store pc)
	if (vm->sp < 17)
		vm->call_stack[vm->sp++] = vm->pc;
	else
		ERROR_EXIT("Attempted push to full VM Stack\n");
	vm->pc = opcode & 0x0FFFu;
	return;
reg_eq_im:
	// 3XNN: skip next instruction if Vx==NN
	if ((vm->v[(opcode & 0x0F00) >> 8u]) == (opcode & 0x00FF))
		vm->pc += 4;
	else
		vm->pc += 2;
	return;
reg_neq_im:
	// 4XNN: skip next instruction if reg!=00NN
	if ((vm->v[(opcode & 0x0F00) >> 8u]) != (opcode & 0x00FF))
		vm->pc += 4;
	else
		vm->pc += 2;
	return;
reg_eq_reg:
	// 5XY0: skip next instruction if Vx==Vy
	for (;;) {
		uint8_t vx = vm->v[(opcode & 0x0F00 >> 8)];
		uint8_t vy = vm->v[(opcode & 0x00F0 >> 4)];
		if (vx == vy)
			vm->pc += 4;
		else
			vm->pc += 2;
		return;
	}
load_halfword:
	// 6XNN: set Vx to 00NN
	vm->v[(opcode & 0x0F00u) >> 8] = (uint8_t)opcode & 0x00FFu;
	vm->pc += 2;
	return;
add_halfword:
	// 7XNN: add 00NN to Vx without affecting carry.
	vm->v[(opcode & 0x0F00u) >> 8] += (uint8_t)opcode & 0x00FFu;
	vm->pc += 2;
	return;
math:
	switch (opcode & 0x000Fu) {
	case 0x0:
		// 8XY0: set Vx=Vy
		vm->v[(opcode & 0x0F00u) >> 8] = vm->v[opcode & 0x00F0u >> 4];
		break;
	case 0x1:
		// 8XY1: Vx = Vx | Vy
		vm->v[(opcode & 0x0F00) >> 8] |= vm->v[(opcode & 0x00F0u) >> 4];
		break;
	case 0x2:
		// 8XY2: Vx = Vx & Vy
		vm->v[(opcode & 0x0F00u) >> 8u] &= vm->v[(opcode & 0x00F0u) >> 4];
		break;
	case 0x3:
		// 8XY3: Vx = Vx ^ Vy
		vm->v[(opcode & 0x0F00u) >> 8u] ^= vm->v[(opcode & 0x00F0u) >> 4];
		break;
	case 0x4:
		// 8XY4: Vx+=vy, carry-aware
		do {
			uint8_t vx = vm->v[(opcode & 0x0F00) >> 8];
			uint8_t vy = vm->v[(opcode & 0x00F0) >> 4];
			int tmp = vx + vy;
			vm->v[0xF] = (tmp > UINT8_MAX) ? 1 : 0;
			vm->v[(opcode & 0x0F00) >> 8] = vx + vy;
		} while (0);
		break;
	case 0x5:
		// 8XY5: borrow-aware sub (n.b. VF set to !borrow)
		do {
			uint8_t vx = vm->v[(opcode & 0x0F00) >> 8];
			uint8_t vy = vm->v[(opcode & 0x00F0) >> 4];
			vm->v[0xF] = (vx > vy) ? 1 : 0;
			vm->v[(opcode & 0x0F00) >> 8] = vx - vy;
		} while (0);
		break;
	case 0x6:
		// 8XY6: euclidean division by two; set VF if remainder
		// i.e. arithmetic right shift
		// store lsb of Vx in VF
		vm->v[0xF] = vm->v[(opcode & 0x0F00) >> 8] & 1;
		vm->v[(opcode & 0x0F00) >> 8] >>= 1;
		break;
	case 0x7:
		// 8XY7: same as 0x5, but Vy-Vx instead of Vx-Vy
		do {
			uint8_t vx = vm->v[(opcode & 0x0F00) >> 8];
			uint8_t vy = vm->v[(opcode & 0x00F0) >> 4];
			int tmp = vy - vx;
			vm->v[0xF] = (tmp < 0) ? 1 : 0xF;
			vm->v[(opcode & 0x00F0) >> 8] = vy - vx;
		} while (0);
		break;
	case 0xE:
		// 8XYE: multiplication by two; set VF if remainder
		// i.e. arithmetic left shift
		// store msb in vf
		vm->v[0xF] = vm->v[(opcode & 0x0F00) >> 8] & 0x80;
		vm->v[(opcode & 0x0F00) >> 8] <<= 1;
		break;
	default:
		ERROR_EXIT("Unknown opcode")
	}
	vm->pc += 2;
	return;
reg_neq_reg:
	// 9XY0: skip if Vx!=Vy
	for (;;) {
		uint8_t vx = vm->v[(opcode & 0x0F00 >> 8)];
		uint8_t vy = vm->v[(opcode & 0x00F0 >> 4)];
		if (vx != vy)
			vm->pc += 4;
		else
			vm->pc += 2;
		return;
	}
set_idx:
	// ANNN
	vm->idx = opcode & 0x0FFF;
	vm->pc += 2;
	return;
jump_idx_plus_reg:
	// BNNN
	vm->pc = vm->v[0] + (opcode & 0x0FFF);
	return;
random_and:
	// CXNN: set Vx to NN & random number 0-225
	vm->v[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF) & (rand() % 255);
	vm->pc += 2;
	return;
draw:
	vm->draw_flag = true;
	draw_sprite(vm, (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4,
				(opcode & 0x00F));
	vm->pc += 2;
	return;
exxx_keyops:
	switch (opcode & 0x00FF) {
	case 0x9E:
		// EX9E: Skip next instruction if key stored in Vx pressed
		if (vm->keyboard[vm->v[opcode & 0x0F00 >> 8]])
			vm->pc += 4;
		else
			vm->pc += 2;
		return;
	case 0xA1:
		// EX9E: Skip next instruction if key stored in Vx NOT pressed
		if (!(vm->keyboard[vm->v[opcode & 0x0F00 >> 8]]))
			vm->pc += 4;
		else
			vm->pc += 2;
		return;
	default:
		ERROR_EXIT("Unknown opcode")
	}
fxxx_ops:
	switch (opcode & 0x00FF) {
	case 0x07:
		// FX07: Set Vx to value of delay timer
		vm->v[(opcode & 0x0F00) >> 8] = vm->delay_timer;
		break;
	case 0x0A:
		// FX0A: Blocking I/O
		vm->v[(opcode & 0x0F00) >> 8] = await_keypress();
		break;
	case 0x15:
		// FX15: Set delay timer to the value of Vx
		vm->delay_timer = vm->v[(opcode & 0x0F00) >> 8];
		break;
	case 0x18:
		// FX18: Set sound timer to the value of Vx
		vm->sound_timer = vm->v[(opcode & 0x0F00) >> 8];
		break;
	case 0x1E:
	}
	vm->pc += 2;
	return;
}