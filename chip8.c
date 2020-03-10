//
// Created by bigfootlives on 2020-03-09.
//

#include "chip8.hpp"
#include <stdbool.h>

static unsigned char memory[4096u];
static unsigned char v[16u]; // cpu registers
static unsigned short index_reg;
static unsigned short pc;  // program counter
// static std::stack<unsigned short, std::array<unsigned short, 48u>> stack;
static unsigned short sp;  // stack pointer
static bool screen[64][32];
static unsigned char delay_timer;  // delay register; should be 60hz
static unsigned char sound_timer;  // same as above
//static std::array<bool, 16> key;  // hex keyboard - some kind of enum?
