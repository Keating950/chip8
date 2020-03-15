#ifndef CHIP8_H
#define CHIP8_H
#include <stdbool.h>

typedef struct CHIP8_STACK {
	short head;
	unsigned short contents[0x30];
} chip8_stack;

typedef struct CHIP8_VM_T {
	unsigned char internal_mem[0x200];
	//  unsigned char rom[0x800];
	unsigned char *rom;
	unsigned char v[0xFU]; // cpu registers
	unsigned short index_reg;
	unsigned short pc; // program counter
	chip8_stack stack; // 48 high
	unsigned short sp; // stack pointer
	bool screen[0x40][0x20];
	unsigned char delay_timer; // delay register; should be 60hz
	unsigned char sound_timer; // same as above
	bool keyboard[0x10]; // hex keyboard - better as some kind of enum?
	bool draw_flag;
} chip8_vm;

chip8_vm initialize_chip8();

void stack_push(unsigned short val, chip8_vm *vm);

unsigned short stack_pop(chip8_stack *stack);

void load_rom(const char *path, chip8_vm *vm);

unsigned short vm_cycle(chip8_vm *vm);

#endif // CHIP8_H
