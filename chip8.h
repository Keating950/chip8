#ifndef CHIP8_H
#define CHIP8_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint8_t mem[0x1000];
	uint8_t v[0x10];
	uint16_t idx;
	uint16_t pc;
	uint16_t stack[0x10];
	uint8_t sp;
	uint32_t screen[0x40 * 0x20];
	int delay_timer;
	int sound_timer; // both 60hz
	int keyboard[0x10];
} chip8_vm;

chip8_vm init_chip8();
void load_rom(const char *path, chip8_vm *vm);
void vm_cycle(chip8_vm *vm, bool key_pressed);

#endif // CHIP8_H
