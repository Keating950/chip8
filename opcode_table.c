//
// Created by bigfootlives on 2020-03-15.
//

#include "opcode_table.h"
#include "chip8.h"
// defining an arbitrary prime to use for hashing
#define P 0x239u
#define TABLE_SIZE 0x2fu
#define compute_hash(key) P *key % TABLE_SIZE

// using 37, a prime, to minimize collisions
void (*function_table[TABLE_SIZE])(chip8_vm *vm, unsigned short opcode);

void execute_opcode(chip8_vm* vm, unsigned short opcode)
{
	function_table[compute_hash(opcode&0x00F)](vm, opcode);
}

extern inline void zeros(chip8_vm *vm, unsigned short opcode);
extern inline void jump(chip8_vm *vm, unsigned short opcode);
extern inline void jump_and_link(chip8_vm *vm, unsigned short opcode);
extern inline void skip_if(chip8_vm *vm, unsigned short opcode);
extern inline void skip_if_ne(chip8_vm *vm, unsigned short opcode);
extern inline void skip_if_reg(chip8_vm *vm, unsigned short opcode);
extern inline void load_val(chip8_vm *vm, unsigned short opcode);
extern inline void add(chip8_vm *vm, unsigned short opcode);
extern inline void eights(chip8_vm *vm, unsigned short opcode);
extern inline void skip_if_reg_ne(chip8_vm *vm, unsigned short opcode);
extern inline void set_index(chip8_vm *vm, unsigned short opcode);