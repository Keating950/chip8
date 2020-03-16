#include "chip8.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

// clang-format off
#define handle_error(msg) \
  do {                    \
    perror(msg);          \
    exit(EXIT_FAILURE);   \
  } while (0)
// clang-format on
#define UINT8_MAX 0xFF
#define execute_opcode(vm, opcode)                                             \
	function_table[compute_hash(opcode & 0x00F)](vm, opcode);

chip8_vm initialize_chip8()
{
	chip8_vm vm = {
		.internal_mem = { 0 },
		.rom = 0,
		.v = { 0 },
		.idx = 0,
		.pc = 0,
		.stack = { 0 },
		.sp = 0,
		.screen = { 0 },
		.delay_timer = 0,
		.sound_timer = 0,
		.keyboard = { false },
		.draw_flag = false,
	};
	return vm;
}

void stack_push(unsigned short val, chip8_vm *vm)
{
	if (vm->sp < 16)
		vm->stack[vm->sp++] = val;
	else
		handle_error("Attempted push to full VM Stack\n");
}

unsigned short stack_pop(chip8_vm *vm)
{
	if (vm->sp > 0)
		return vm->stack[--vm->sp];
	else
		handle_error("Attempted pop from empty VM Stack\n");
}

unsigned short *stack_peep(chip8_vm *vm)
{
	if (vm->sp > 0)
		return &(vm->stack[(vm->sp) - 1]);
	return NULL;
}

void load_rom(const char *path, chip8_vm *vm)
{
	int fd;
	struct stat sb;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		handle_error("fd was -1 after opening rom\n");
	if (fstat(fd, &sb) == -1) /* To obtain file size */
		handle_error("fstat error loading ROM\n");
	if (sb.st_size > 0x800)
		handle_error("ROM size exceeds specification's allocatable"
			     "internal memory\n");
	vm->rom = (unsigned char *)mmap(NULL, 1, PROT_READ, MAP_PRIVATE, fd, 0);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"

void vm_cycle(chip8_vm *vm)
{
	unsigned short opcode = vm->rom[vm->pc] << 8 | vm->rom[vm->pc + 1];
	const void *opcode_handles[] = {
		&&zero,	      &&jump,	    &&jump_and_link, &&reg_eq_im,
		&&reg_neq_im, &&reg_eq_reg, &&load_halfword, &&add_unsigned,
		&&math,	      &&set_idx,
	};
	if (opcode_handles[(opcode & 0xF000) >> 12] != NULL)
		goto *opcode_handles[(opcode & 0xF000) >> 12];
	fprintf(stderr, "Error: Unknown opcode %#X\n", opcode);
	exit(EXIT_FAILURE);

zero:
	switch (opcode & 0x000F) {
	case 0x0000:
		// clear screen
		vm->draw_flag = true;
		//		vm->screen = {false}{false};
		memset(vm->screen, false,
		       sizeof(vm->screen[0][0]) * 0x40 * 0x20);
		vm->pc += 2;
		break;
	case 0x000E:
		// jr $ra
		vm->pc = stack_pop(vm);
		break;
	default:
		// deprecated instruction
		vm->pc += 2;
		break;
	}
jump:
	// jump (don't store pc)
	vm->pc = opcode & 0x0FFF;
	return;
jump_and_link:
	// call subroutine (store pc)
	stack_push(vm->pc, vm);
	vm->pc = opcode & 0x0FFF;
	return;
reg_eq_im:
	// skip next instruction if reg==00NN
	vm->pc += vm->v[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF) ? 4 : 2;
	return;
reg_neq_im:
	// skip next instruction if reg!=00NN
	vm->pc += vm->v[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF) ? 4 : 2;
	return;
reg_eq_reg:
	// skip next instruction if Vx==Vy
	vm->pc += vm->v[(opcode & 0x0F00) >> 8] == vm->v[opcode & 0x00F0] ? 4 :
									    2;
	return;
load_halfword:
	// set Vx to 00NN
	vm->v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
	vm->pc += 2;
	return;
add_unsigned:
	// add 00NN to Vx without affecting carry.
	vm->v[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
	vm->pc += 2;
	return;
math:
	switch (opcode & 0x000F) {
	case 0x0:
		// set Vx=Vy
		vm->v[(opcode & 0x0F00) >> 8] = vm->v[opcode & 0x00F0];
		vm->pc += 2;
		return;
	case 0x1:
		// or
		vm->v[(opcode & 0x0F00) >> 8] = vm->v[(opcode & 0x0F00) >> 8]
						| vm->v[(opcode & 0x00F0) >> 8];
		vm->pc += 2;
		return;
	case 0x2:
		// and
		vm->v[(opcode & 0x0F00) >> 8u]
			= vm->v[(opcode & 0x0F00) >> 8u]
			  & vm->v[(opcode & 0x00F0) >> 8u];
		vm->pc += 2;
		return;
	case 0x3:
		// xor
		vm->v[(opcode & 0x0F00) >> 8u]
			= vm->v[(opcode & 0x0F00) >> 8u]
			  ^ vm->v[(opcode & 0x00F0) >> 8u];
		vm->pc += 2;
		return;
	case 0x4:
		// carry-aware add
		vm->v[0xF]
			= (vm->v[(opcode & 0) >> 8] + vm->v[(opcode & 0) >> 8]
			   > UINT8_MAX);
		vm->v[(opcode & 0) >> 8]
			= (vm->v[(opcode & 0) >> 8] + vm->v[(opcode & 0) >> 8])
			  & 0x00F;
		vm->pc += 2;
		return;
	case 0x5:
		// borrow-aware sub (n.b. VF set to !borrow)
		vm->v[0xF]
			= vm->v[(opcode & 0) >> 8] <= vm->v[(opcode & 0) >> 8];
		vm->v[(opcode & 0) >> 8] -= vm->v[(opcode & 0) >> 8];
		vm->pc += 2;
		return;
	case 0x6:
		// euclidean division by two; set VF if remainder
		// i.e. arithmetic right shift
		vm->v[0xF] = vm->v[(opcode & 0) >> 8] & 1;
		vm->v[(opcode & 0) >> 8] >>= 1;
	case 0x7:
		// same as 0x5, but Vy-Vx instead of Vx-Vy
		vm->v[0xF]
			= vm->v[(opcode & 0) >> 8] >= vm->v[(opcode & 0) >> 8];
		vm->v[(opcode & 0) >> 8] -= vm->v[(opcode & 0) >> 8];
		vm->pc += 2;
		return;
	case 0xE:
		// multiplication by two; set VF if remainder
		// i.e. arithmetic left shift
		vm->v[0xF] = vm->v[(opcode & 0) >> 8] & 1;
		vm->v[(opcode & 0) >> 8] <<= 1;
	}
set_idx:
	// set index to val
	vm->idx = opcode & 0x0FFF;
	vm->pc += 2;
	return;
jump_idx_plus_v:
	vm->pc = vm->v[0] + (opcode & 0x0FFF);
	return;
random_and:
	srand(clock());
	vm->v[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF) & (rand() % 255);
	vm->pc += 2;
	return;
}
#pragma clang diagnostic pop
