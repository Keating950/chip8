#ifndef CHIP8_H
#define CHIP8_H
#include <stdint.h>
#include <stdbool.h>

#define COLS 0x40
#define ROWS 0x20

typedef struct CHIP8_VM_T {
	uint8_t mem[0x1000];
	uint8_t v[0x10]; // cpu registers
	uint16_t idx;
	uint16_t pc; // program counter
	uint16_t call_stack[0x10];
	int sp; // stack pointer
	uint32_t screen[COLS * ROWS];
	int delay_timer; // delay register; should be 60hz
	int sound_timer; // same as above
	bool keyboard[0x10]; // hex keyboard - better as some kind of enum?
	bool draw_flag;
} chip8_vm;

chip8_vm init_chip8();

void load_rom(const char *path, chip8_vm *vm);

void vm_cycle(chip8_vm *vm);

#endif // CHIP8_H
