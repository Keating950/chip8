//
// Created by Keating Reid on 2020-03-09.
//

#ifndef CHIP8_COMPONENTS_H
#define CHIP8_COMPONENTS_H

/*
    0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
    0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
    0x200-0xFFF - Program ROM and work RAM
 */

typedef unsigned short opcode_t;
typedef struct CHIP8_T
{
    unsigned char memory[4096u];
    unsigned char v[16u]; // cpu registers
    unsigned short index_reg;
    unsigned short pc;  // program counter
    std::stack<unsigned short, std::array<unsigned short, 48u>> stack;
    unsigned short sp;  // stack pointer
    std::array<std::array<bool, 64>, 32> screen;
    unsigned char delay_timer;  // delay register; should be 60hz
    unsigned char sound_timer;  // same as above
    std::array<bool, 16> key;  // hex keyboard
} chip8;

#endif //CHIP8_COMPONENTS_H
