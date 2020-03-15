#include "chip8.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// clang-format off
#define handle_error(msg) \
  do {                    \
    perror(msg);          \
    exit(EXIT_FAILURE);   \
  } while (0)
// clang-format on

chip8_vm initialize_chip8()
{
	chip8_vm vm = {
		//    .opcode = 0,
		.internal_mem = { 0 },
		//    .rom = {0},
		.rom = 0,
		.v = { 0 },
		.index_reg = 0,
		.pc = 0,
		.stack = { -1, { 0 } },
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
	chip8_stack *stack = &vm->stack;
	if (stack->head < 48)
		stack->contents[stack->head++] = val;
	else
		handle_error("ERROR: Attempted push to full VM Stack\n");
}

unsigned short stack_pop(chip8_stack *stack)
{
	if (stack->head >= 0)
		return stack->contents[stack->head--];
	else
		handle_error("ERROR: Attempted pop from empty VM Stack\n");
}

unsigned short stack_peep(chip8_stack *stack)
{
	if (stack->head >= 0)
		return stack->contents[stack->head];
	return -1;
}

void load_rom(const char *path, chip8_vm *vm)
{
	int fd;
	struct stat sb;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		handle_error("Error opening rom\n");
	if (fstat(fd, &sb) == -1) /* To obtain file size */
		handle_error("fstat error loading ROM\n");
	if (sb.st_size > 0x800)
		handle_error("ROM size exceeds specification's allocatable"
			     "internal memory\n");
	vm->rom = (unsigned char *)mmap(NULL, 1, PROT_READ, MAP_PRIVATE, fd, 0);
}

unsigned short vm_cycle(chip8_vm *vm)
{
	unsigned short opcode = vm->rom[vm->pc] << 8 | vm->rom[vm->pc + 1];
	unsigned short external_action = 0;

	switch (opcode & 0xF000) {
	case 0x0000:
		// deprecated instruction
		vm->pc += 2;
		break;
	case 0x00E0:
		// clear screen
		external_action = 0x00E0;
		vm->pc += 2;
		break;
	case 0x00EE:
		// jr $ra
		vm->pc = stack_pop(&vm->stack);
		break;
	case 0x1000:
		// jump (don't store pc)
		vm->pc = opcode & 0xFFF;
		break;
	case 0x2000:
		// call subroutine (store pc)
		stack_push(vm->pc, vm);
		vm->pc = opcode & 0xFFF;
		break;
	case 0x3000:
		// skip next instruction if reg==00NN
		vm->pc += vm->v[opcode & 0x0F00] == (opcode & 0x00FF) ? 4 : 2;
		break;
	case 0x4000:
		// skip next instruction if reg!=00NN
		vm->pc += vm->v[opcode & 0x0F00] != (opcode & 0x00FF) ? 4 : 2;
		break;
	case 0x5000:
		// skip next instruction if Vx==Vy
		vm->pc += vm->v[opcode & 0x0F00] == vm->v[opcode & 0x00F0] ? 4 :
									     2;
		break;
	case 0x6000:
		// set Vx to 00NN
		vm->v[opcode & 0x0F00] = opcode & 0x00FF;
		break;
	case 0x7000:
		// add 00NN to Vx
		vm->v[opcode & 0x0F00] += opcode & 0x00FF;
		break;
	case 0x8000:
		// set Vx=Vy
		vm->v[opcode & 0x0F00] = vm->v[opcode & 0x00F0];
		break;
	case 0xA000:
		// set index to val
		vm->index_reg = opcode & 0xFFF;
		vm->pc += 2;
		break;
	default:
		fprintf(stderr, "Error: Unknown opcode %#x\n", opcode);
		exit(EXIT_FAILURE);
	}
	return external_action;
}
