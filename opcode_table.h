//
// Created by bigfootlives on 2020-03-15.
//

#ifndef FUNCTION_TABLE_H
#define FUNCTION_TABLE_H
#include "chip8.h"
#define UINT8_MAX 0xFF

inline void zeros(chip8_vm *vm, unsigned short opcode)
{
	switch (opcode & 0x000F) {
	case 0x0000:
		// clear screen
		vm->draw_instruction = 0x00E0;
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
}

inline void jump(chip8_vm *vm, unsigned short opcode)
{ // 0x1
	vm->pc = opcode & 0xFFF;
}

inline void jump_and_link(chip8_vm *vm, unsigned short opcode)
{ // 0x2
	stack_push(vm->pc, vm);
	vm->pc = opcode & 0xFFF;
}

inline void skip_if(chip8_vm *vm, unsigned short opcode)
{ // 0x3
	vm->pc += vm->v[opcode & 0x0F00] == (opcode & 0x00FF) ? 4 : 2;
}

inline void skip_if_ne(chip8_vm *vm, unsigned short opcode)
{ // 0x4
	vm->pc += vm->v[opcode & 0x0F00] != (opcode & 0x00FF) ? 4 : 2;
}

inline void skip_if_reg(chip8_vm *vm, unsigned short opcode)
{ // 0x5
	vm->pc += vm->v[opcode & 0x0F00] == vm->v[opcode & 0x00F0] ? 4 : 2;
}

inline void load_val(chip8_vm *vm, unsigned short opcode)
{ // 0x6
	vm->v[opcode & 0x0F00] = opcode & 0x00FF;
	vm->pc += 2;
}

inline void add(chip8_vm *vm, unsigned short opcode)
{
	vm->v[opcode & 0x0F00] += opcode & 0x00FF;
	vm->pc += 2;
}

inline void eights(chip8_vm *vm, unsigned short opcode)
{
	switch (opcode & 0x000F) {
	case 0x0:
		// set Vx=Vy
		vm->v[opcode & 0x0F00] = vm->v[opcode & 0x00F0];
		vm->pc += 2;
		break;
	case 0x1:
		// or
		vm->v[opcode & 0x0F00]
			= vm->v[opcode & 0x0F00] | vm->v[opcode & 0x00F0];
		vm->pc += 2;
		break;
	case 0x2:
		// and
		vm->v[opcode & 0x0F00]
			= vm->v[opcode & 0x0F00] & vm->v[opcode & 0x00F0];
		vm->pc += 2;
		break;
	case 0x3:
		// xor
		vm->v[opcode & 0x0F00]
			= vm->v[opcode & 0x0F00] ^ vm->v[opcode & 0x00F0];
		vm->pc += 2;
		break;
	case 0x4:
		// carry-aware add
		vm->v[0xF] = (vm->v[opcode & 0x0F00] + vm->v[opcode & 0x00F0]
			      > UINT8_MAX);
		vm->v[opcode & 0x0F00]
			= (vm->v[opcode & 0x0F00] + vm->v[opcode & 0x00F0])
			  & 0x00F;
		vm->pc += 2;
		break;
	case 0x5:
		// borrow-aware sub (n.b. VF set to !borrow)
		vm->v[0xF] = vm->v[opcode & 0x0F00] <= vm->v[opcode & 0x00F0];
		vm->v[opcode & 0x0F00] -= vm->v[opcode & 0x00F0];
		vm->pc += 2;
		break;
	case 0x6:
		// euclidean division by two; set VF if remainder
		// i.e. arithmetic right shift
		vm->v[0xF] = vm->v[opcode & 0x0F00] & 1;
		vm->v[opcode & 0x0F00] >>= 1;
	case 0x7:
		// same as 0x5, but Vy-Vx instead of Vx-Vy
		vm->v[0xF] = vm->v[opcode & 0x0F00] >= vm->v[opcode & 0x00F0];
		vm->v[opcode & 0x00F0] -= vm->v[opcode & 0x0F00];
		vm->pc += 2;
		break;
	case 0xE:
		// multiplication by two; set VF if remainder
		// i.e. arithmetic left shift
		vm->v[0xF] = vm->v[opcode & 0x0F00] & 1;
		vm->v[opcode & 0x0F00] <<= 1;
	}
}

inline void skip_if_reg_ne(chip8_vm *vm, unsigned short opcode)
{
	vm->pc += vm->v[opcode & 0x0F00] != vm->v[opcode & 0x00F0] ? 4 : 2;
}

inline void set_index(chip8_vm *vm, unsigned short opcode)
{
	vm->pc += vm->v[opcode & 0x0F00] != vm->v[opcode & 0x00F0] ? 4 : 2;
}
#endif //FUNCTION_TABLE_H
