#ifndef CHIP8_H
#define CHIP8_H
#include <stdbool.h>

typedef struct CHIP8_VM_T {
	unsigned char internal_mem[0x200];
	//  unsigned char rom[0x800];
	unsigned char *rom;
	unsigned char v[0x10]; // cpu registers
	unsigned short idx;
	unsigned short pc; // program counter
	unsigned short call_stack[16];
	short sp; // stack pointer
	bool screen[0x40][0x20];
	short delay_timer; // delay register; should be 60hz
	short sound_timer; // same as above
	bool keyboard[0x10]; // hex keyboard - better as some kind of enum?
	bool draw_flag;
} chip8_vm;

chip8_vm init_chip8();

void load_rom(const char *path, chip8_vm *vm);

void print_rom(chip8_vm vm);

void vm_cycle(chip8_vm *vm);

#endif // CHIP8_H
