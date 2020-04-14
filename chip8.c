#include "chip8.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#define ERROR_EXIT(msg)                                                   \
	do {                                                              \
		perror(msg);                                              \
		exit(EXIT_FAILURE);                                       \
	} while (0)

#define ROWS 0x40
#define COLS 0x20
#define ON 0xFFFFFFFF
#define OFF 0

chip8_vm init_chip8()
{
	srand(clock()); // for opcode requiring RNG
	chip8_vm vm = {
		.internal_mem = { 0 },
		.rom = 0,
		.v = { 0 },
		.idx = 0,
		.pc = 0,
		.call_stack = { 0 },
		.sp = 0,
		.screen = {{ OFF }},
		.delay_timer = 0,
		.sound_timer = 0,
		.keyboard = { false },
		.draw_flag = false,
	};
	return vm;
}

void load_rom(const char *path, chip8_vm *vm)
{
	int fd;
	struct stat sb;
	int *map;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		ERROR_EXIT("fd was -1 after opening rom\n");
	if (fstat(fd, &sb) == -1) /* To obtain file size */
		ERROR_EXIT("fstat error loading ROM\n");
	if (sb.st_size > 0x800)
		ERROR_EXIT(
			"ROM size exceeds specification's allocatable internal memory\n");
	map = mmap(NULL, 1, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED)
		ERROR_EXIT("Map failed");
	else
		vm->rom = (unsigned char *) map;
}

void print_rom(chip8_vm vm)
{
	do {
		for (int i = 0; i < 3; i++) {
			unsigned short opcode =
				vm.rom[vm.pc] << 8u |
				vm.rom[vm.pc +
				       1]; // NOLINT(hicpp-signed-bitwise)
			unsigned int instruction = opcode & 0xf000u;
			printf("%#X  ", instruction);
			vm.pc += 2;
		}
		printf("\n");
	} while (vm.pc < 800);
}

static void push(chip8_vm *vm)
{
	if (vm->sp < 16)
		vm->call_stack[vm->sp++] = vm->pc;
	else
		ERROR_EXIT("Attempted push to full VM Stack\n");
}

static unsigned short pop(chip8_vm *vm)
{
	if (vm->sp > 0)
		return vm->call_stack[--vm->sp];
	else
		ERROR_EXIT("Attempted pop from empty VM Stack\n");
}

static void xor_screen(chip8_vm *vm, int x, int y, int height)
{
	const int topmost_row = vm->v[y] - height / 2;
	const int bottommost_row = vm->v[y] + height / 2;
	const int leftmost_col = vm->v[x] - 4;
	const int rightmost_col = vm->v[x] + 4;

	vm->v[0xF] = 0; // reset the collision flag
	for (int i = topmost_row; i < bottommost_row; i++) {
		for (int j = leftmost_col; j < rightmost_col; j++) {
			const uint32_t tmp = vm->screen[i][j];
			vm->screen[i][j] ^= vm->screen[i][j];
			if (tmp == ON && vm->screen[i][j] == OFF)
				vm->v[0xF] = 1;
		}
	}
}

void vm_cycle(chip8_vm *vm)
{
	const unsigned short opcode =
		vm->rom[vm->pc] << 8 | vm->rom[vm->pc+1];
	// FLAG
	fprintf(stderr, "%04X\n", opcode);
	/* FLAG: Try declaring this static -- suspect it might lead to an error
	if vm_cycle's location in memory moves, but worth investigating */
	static const void *opcode_handles[] = {
		&&zero_ops,
		&&jump,
		&&jump_and_link,
		&&reg_eq_im,
		&&reg_neq_im,
		&&reg_eq_reg,
		&&load_halfword,
		&&add_unsigned,
		&&math,
		&&set_idx,
		&&jump_idx_plus_reg,
		&&random_and,
		&&draw,
		&&exxx_keyops,
		&&fxxx_ops,
	    };

	if (opcode_handles[(opcode & 0xF000u) >> 12])
		goto *opcode_handles[(opcode & 0xF000u) >> 12];
	else {
		fprintf(stderr, "Error: Unknown opcode %#X\n", opcode);
		exit(EXIT_FAILURE);
	}

zero_ops:
	switch (opcode & 0x00FFu) {
	case 0x00E0:
		// clear screen
		vm->draw_flag = true;
		for (int i=0; i<0x20; i++)
			memset(vm->screen+i, OFF, 0x40);
		vm->pc += 2;
		return;
	case 0x00EE:
		// jr $ra
		vm->pc = pop(vm);
		return;
	default:
		// deprecated instruction
		vm->pc += 2;
		return;
	}
jump:
	// jump (don't store pc)
	vm->pc = opcode & 0x0FFFu;
	return;
jump_and_link:
	// call subroutine (store pc)
	push(vm);
	vm->pc = opcode & 0x0FFFu;
	return;
reg_eq_im:
	// skip next instruction if reg==00NN
	vm->pc +=
		vm->v[(opcode & 0x0F00u) >> 8] == (opcode & 0x00FFu) ? 4 : 2;
	return;
reg_neq_im:
	// skip next instruction if reg!=00NN
	vm->pc +=
		vm->v[(opcode & 0x0F00u) >> 8] != (opcode & 0x00FFu) ? 4 : 2;
	return;
reg_eq_reg:
	// skip next instruction if Vx==Vy
	vm->pc += vm->v[(opcode & 0x0F00u) >> 8] == vm->v[opcode & 0x00F0u] ?
			  4 :
			  2;
	return;
load_halfword:
	// set Vx to 00NN
	vm->v[(opcode & 0x0F00u) >> 8] = opcode & 0x00FFu;
	vm->pc += 2;
	return;
add_unsigned:
	// add 00NN to Vx without affecting carry.
	vm->v[(opcode & 0x0F00u) >> 8] += opcode & 0x00FFu;
	vm->pc += 2;
	return;
math:
	switch (opcode & 0x000Fu) {
	case 0x0:
		// set Vx=Vy
		vm->v[(opcode & 0x0F00u) >> 8] = vm->v[opcode & 0x00F0u];
		vm->pc += 2;
		return;
	case 0x1:
		// or
		vm->v[(opcode & 0x0F00) >> 8] =
			vm->v[(opcode & 0x0F00u) >> 8] |
			vm->v[(opcode & 0x00F0u) >> 8];
		vm->pc += 2;
		return;
	case 0x2:
		// and
		vm->v[(opcode & 0x0F00u) >> 8u] =
			vm->v[(opcode & 0x0F00u) >> 8] &
			vm->v[(opcode & 0x00F0u) >> 8];
		vm->pc += 2;
		return;
	case 0x3:
		// xor
		vm->v[(opcode & 0x0F00) >> 8u] =
			vm->v[(opcode & 0x0F00u) >> 8] ^
			vm->v[(opcode & 0x00F0u) >> 8];
		vm->pc += 2;
		return;
	case 0x4:
		// carry-aware add
		vm->v[0xf] =
			(unsigned char) (vm->v[(opcode & 0) >> 8] +
						 vm->v[(opcode & 0) >> 8] >
					 UINT8_MAX);
		vm->v[(opcode & 0) >> 8] = (vm->v[(opcode & 0) >> 8] +
					    vm->v[(opcode & 0) >> 8]) &
					   0x00Fu;
		vm->pc += 2;
		return;
	case 0x5:
		// borrow-aware sub (n.b. VF set to !borrow)
		vm->v[0xF] =
			(unsigned char) (vm->v[(opcode & 0x0F00u) >> 8] <=
					 vm->v[(opcode & 0x00F0u) >> 4]);
		vm->v[(opcode & 0) >> 8] -= vm->v[(opcode & 0) >> 8];
		vm->pc += 2;
		return;
	case 0x6:
		// euclidean division by two; set VF if remainder
		// i.e. arithmetic right shift
		vm->v[0xF] = vm->v[(opcode & 0) >> 8] & 1;
		vm->v[(opcode & 0x00FFu) >> 8] >>= 1;
		vm->pc += 2;
		return;
	case 0x7:
		// same as 0x5, but Vy-Vx instead of Vx-Vy
		vm->v[0xF] =
			(unsigned char) (vm->v[(opcode & 0x0F00u) >> 8] >=
					 vm->v[(opcode & 0x00F0u) >> 4]);
		vm->v[(opcode & 0x0F00u) >> 8] -=
			vm->v[(opcode & 0x00F0u) >> 4];
		vm->pc += 2;
		return;
	case 0xE:
		// multiplication by two; set VF if remainder
		// i.e. arithmetic left shift
		vm->v[0xF]= vm->v[(opcode & 0x0F00u) >> 8] & 0x80u; // store msb in VF
		vm->v[(opcode & 0x0F00u) >> 8] <<= 1;
		vm->pc += 2;
		return;
	}
set_idx:
	vm->idx = opcode & 0x0FFF;
	vm->pc += 2;
	return;
jump_idx_plus_reg:
	vm->pc = vm->v[0] + (opcode & 0x0FFF);
	return;
random_and:
	vm->v[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF) & (rand() % 255);
	vm->pc += 2;
	return;
draw:
	vm->draw_flag = true;
	xor_screen(vm, (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4,
		   (opcode & 0x00F));
	vm->pc += 2;
	return;
exxx_keyops:
	// EX9E
	// EXA1
	return;
fxxx_ops:
	return;

}
