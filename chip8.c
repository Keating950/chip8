#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "chip8.h"
#include "util.h"

#define VX vm->v[(opcode & 0x0F00) >> 8]
#define VY vm->v[(opcode & 0x00F0) >> 4]

const uint8_t chip8_fontset[0x50] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80 // F
};

chip8_vm init_chip8()
{
	srand(clock()); // for opcode requiring RNG
	chip8_vm vm = {
		.mem = { 0 },
		.v = { 0 },
		.idx = 0,
		.pc = 200,
		.stack = { 0 },
		.sp = 15,
		.screen = { 0 },
		.delay_timer = 0,
		.sound_timer = 0,
		.keyboard = { 0 },
		.draw_flag = 0,
	};
	memcpy(&vm.mem, chip8_fontset, 0x50);
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

static void print_keys(chip8_vm *vm)
{
	printf("[%c] ", vm->keyboard[0] ? '.' : '0');
	for (int i = 1; i < 0xF; i++)
		printf("[%x] ", vm->keyboard[i] ? i : 0);
	puts("\n");
}

static void draw_sprite(chip8_vm *vm, uint16_t opcode)
{
	int x, y;
	uint8_t pixel;
	int height = opcode & 0x000Fu;
	vm->v[0xF] = 0;

	for (y = 0; y < height && (y + height) < ROWS; y++) {
		pixel = vm->mem[vm->idx + y];
		for (x = 0; x < 8; x++) {
			//			if ((x + VX) > COLS)
			//				break;
			if (pixel & (0x80 >> x)) {
				if (vm->screen[x + VX + (y + VY) * 64])
					vm->v[0xF] = 1;
				vm->screen[x + VX + (y + VY) * 64] ^= UINT32_MAX;
			}
		}
	}
	NOP;
}

void vm_cycle(chip8_vm *vm)
{
	const unsigned short opcode = vm->mem[vm->pc] << 8 | vm->mem[vm->pc + 1];
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
		for (int i = 0; i < ROWS; i++)
			memset(vm->screen + i * ROWS, 0, COLS);
		vm->draw_flag = 1;
		vm->pc += 2;
		return;
	case 0xEE:
		// jr $ra
		if (vm->sp < 15)
			vm->pc = vm->stack[vm->sp++] + 2;
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
	if (vm->sp > 0)
		vm->stack[--vm->sp] = vm->pc;
	else
		ERROR_EXIT("Attempted push to full VM Stack\n");
	vm->pc = opcode & 0x0FFFu;
	return;
reg_eq_im:
	// 3XNN: skip next instruction if Vx==NN
	if ((VX) == (opcode & 0xFF))
		vm->pc += 4;
	else
		vm->pc += 2;
	return;
reg_neq_im:
	// 4XNN: skip next instruction if reg!=00NN
	if ((VX) != (opcode & 0xFF))
		vm->pc += 4;
	else
		vm->pc += 2;
	return;
reg_eq_reg:
	// 5XY0: skip next instruction if Vx==Vy
	if (VX == VY)
		vm->pc += 4;
	else
		vm->pc += 2;
	return;
load_halfword:
	// 6XNN: set Vx to 00NN
	VX = (uint8_t)opcode & 0x00FFu;
	vm->pc += 2;
	return;
add_halfword:
	// 7XNN: add 00NN to Vx without affecting carry.
	VX += (uint8_t)opcode & 0x00FFu;
	vm->pc += 2;
	return;
math:
	switch (opcode & 0x000Fu) {
	case 0x0:
		// 8XY0: set Vx=Vy
		VX = VY;
		break;
	case 0x1:
		// 8XY1: Vx = Vx | Vy
		VX |= VY;
		break;
	case 0x2:
		// 8XY2: Vx = Vx & Vy
		VX &= VY;
		break;
	case 0x3:
		// 8XY3: Vx = Vx ^ Vy
		VX ^= VY;
		break;
	case 0x4:
		// 8XY4: Vx+=vy, carry-aware
		vm->v[0xF] = (((uint32_t)VX + VY) > UINT8_MAX) ? 1 : 0;
		VX += VY;
		break;
	case 0x5:
		// 8XY5: borrow-aware sub (n.b. VF set to !borrow)
		vm->v[0xF] = (VX > VY) ? 1 : 0;
		VX += VY;
		break;
	case 0x6:
		// 8XY6: euclidean division by two; set VF if remainder
		// i.e. arithmetic right shift
		// store lsb of Vx in VF
		vm->v[0xF] = VX & 1;
		VX >>= 1;
		break;
	case 0x7:
		// 8XY7: same as 0x5, but Vy-Vx instead of Vx-Vy
		do {
			uint32_t tmp = VY - VX;
			vm->v[0xF] = (tmp < 0) ? 1 : 0;
			VY -= VX;
		} while (0);
		break;
	case 0xE:
		// 8XYE: multiplication by two; set VF if remainder
		// i.e. arithmetic left shift
		// store msb in vf
		vm->v[0xF] = VX & 0x80;
		VX <<= 1;
		break;
	default:
		ERROR_EXIT("Unknown opcode 8xxx");
	}
	vm->pc += 2;
	return;
reg_neq_reg:
	// 9XY0: skip if Vx!=Vy
	for (;;) {
		if (VX != VY)
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
	VX = (opcode & 0x00FF) & (rand() % 255);
	vm->pc += 2;
	return;
draw:
	vm->draw_flag = 1;
	draw_sprite(vm, opcode);
	vm->pc += 2;
	return;
exxx_keyops:
	switch (opcode & 0x00FF) {
	case 0x9E:
		// EX9E: Skip next instruction if key stored in Vx pressed
		if (vm->keyboard[(VX)])
			vm->pc += 4;
		else
			vm->pc += 2;
		return;
	case 0xA1:
		// EX9E: Skip next instruction if key stored in Vx NOT pressed
		if (!(vm->keyboard[(VX)]))
			vm->pc += 4;
		else
			vm->pc += 2;
		return;
	default:
		ERROR_EXIT("Unknown opcode Exxx");
	}
fxxx_ops:
	switch (opcode & 0x00FF) {
	case 0x07:
		// FX07: Set Vx to value of delay timer
		VX = vm->delay_timer;
		vm->pc += 2;
		break;
	case 0x0A:
		// FX0A: Blocking I/O
		// frames continue but pc doesnt advance until a key is pressed
		for (size_t i = 0; i < LEN(vm->keyboard); i++) {
			if (vm->keyboard[i]) {
				VX = i;
				vm->pc += 2;
				return;
			}
		}
		break;
	case 0x15:
		// FX15: Set delay timer to the value of Vx
		vm->delay_timer = VX;
		vm->pc += 2;
		break;
	case 0x18:
		// FX18: Set sound timer to the value of Vx
		vm->sound_timer = VX;
		vm->pc += 2;
		break;
	case 0x1E:
		// FX1E: add Vx to idx. Set 0xF if there is overflow.
		vm->v[0xF] = (((uint32_t)VX + vm->idx) > UINT16_MAX) ? 1 : 0;
		vm->idx += VX;
		vm->pc += 2;
		break;
	case 0x29:
		// FX29: set idx to location char in Vx's sprite
		vm->idx = VX * 5u;
		vm->pc += 2;
		break;
	case 0x33:
		// FX33: Store BCD representation of Vx in idx through idx+3
		vm->mem[vm->idx] = VX / 100;
		vm->mem[vm->idx + 1] = (VX / 10) % 10;
		vm->mem[vm->idx + 2] = VX % 10;
		vm->pc += 2;
		break;
	case 0x55:
		// FX55: register dump: stores V0 through VX in memory,
		// starting at idx
		for (size_t i = 0; i <= ((opcode & 0x0F00u) >> 8u); i++) {
			vm->mem[(vm->idx) + i] = vm->v[i];
		}
		vm->pc += 2;
		break;
	case 0x65:
		// FX55: register load: loads V0 through VX from memory,
		// starting from idx
		for (size_t i = 0; i <= ((opcode & 0x0F00u) >> 8u); i++) {
			vm->v[i] = vm->mem[(vm->idx) + i];
		}
		vm->pc += 2;
		break;
	default:
		ERROR_EXIT("Unknown opcode Fxxx");
	}
	return;
}
