#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "chip8.h"
#include "util.h"

#define COLS (0x40)
#define ROWS (0x20)
#define OP_FMT "%#06x"
#define VX (vm->v[(opcode & 0x0F00) >> 8])
#define VY (vm->v[(opcode & 0x00F0) >> 4])
#define ERROR_BUF_SIZE 256

static const uint8_t font_set[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

static char error_buf[256];

static void set_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(error_buf, 256, fmt, args);
    if (len < 256)
        error_buf[len] = 0;
    va_end(args);
}

const char *chip8_get_error(void)
{
    return error_buf;
}

chip8_vm *init_chip8()
{
    srand(clock()); // for opcode requiring RNG
    chip8_vm *vm = calloc(1, sizeof(chip8_vm));
    vm->pc = 0xC8;
    vm->sp = 0xF;
    memcpy(vm->mem, font_set, LEN(font_set));
    return vm;
}

int load_rom(const char *path, chip8_vm *vm)
{
    size_t rom_size;
    size_t bytes_read;
    FILE *f = fopen(path, "rb");
    if (!f) {
        set_error("Failed to open file");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    rom_size = ftell(f);
    if (rom_size > 0x800) {
        set_error("Rom is too large");
        return 1;
    }
    rewind(f);
    bytes_read = fread((vm->mem + 0x200), sizeof(uint8_t), rom_size, f);
    fclose(f);
    if (bytes_read != rom_size) {
        set_error("Rom is too large");
        return 1;
    }
    return 0;
}

static void draw_sprite(chip8_vm *vm, uint16_t opcode)
{
#define VMPIXEL(X, Y) (vm->screen[Y * COLS + X])
    int x, y;
    uint8_t sprite_pixel;
    int height = opcode & 0xFu;
    vm->v[0xF] = 0;

    for (y = 0; y < height && (y + height) < ROWS; y++) {
        if ((y + VY) >= ROWS) {
            break;
        }
        sprite_pixel = vm->mem[vm->idx + y];
        for (x = 0; x < 8; x++) {
            if ((x + VX) >= COLS) {
                break;
            }
            if (sprite_pixel & (0x80 >> x)) {
                if (VMPIXEL((x + VX), (y + VY))) {
                    vm->v[0xF] = 1;
                }
                VMPIXEL((x + VX), (y + VY)) ^= UINT32_MAX;
            }
        }
    }
#undef VMPIXEL
}

int vm_cycle(chip8_vm *vm, bool key_pressed)
{
    static const void *opcode_handles[] = {
        &&zero_ops,   &&jump,        &&jump_and_link, &&reg_eq_im,
        &&reg_neq_im, &&reg_eq_reg,  &&load_halfword, &&add_halfword,
        &&math,       &&reg_neq_reg, &&set_idx,       &&jump_idx_plus_reg,
        &&random_and, &&draw,        &&exxx_keyops,   &&fxxx_ops,
    };
    const uint16_t opcode = vm->mem[vm->pc] << 8 | vm->mem[vm->pc + 1];
    if (likely(((opcode & 0xF000u) >> 12) < LEN(opcode_handles)))
        goto *opcode_handles[(opcode & 0xF000u) >> 12];
    else {
        set_error("Unknown opcode " OP_FMT, opcode);
        return 1;
    }

zero_ops:
    switch (opcode & 0x00FFu) {
    case 0xE0:
        // Clear screen
        memset(vm->screen, 0, ROWS * COLS * sizeof(uint32_t));
        vm->pc += 2;
        return 0;
    case 0xEE:
        // jr $ra
        if (unlikely(vm->sp > 15)) {
            set_error("Attempted pop from empty VM Stack\n");
            return 1;
        }
        vm->pc = vm->stack[vm->sp++] + 2;
        return 0;
    default:
        // 0NNN: Deprecated instruction
        vm->pc += 2;
        return 0;
    }
jump:
    // 1NNN: Jump without storing pc
    vm->pc = opcode & 0x0FFFu;
    return 0;
jump_and_link:
    // 2NNN: Call subroutine, storing pc
    if (unlikely(vm->sp <= 0)) {
        set_error("Attempted push to full VM Stack\n");
        return 1;
    }
    vm->stack[--vm->sp] = vm->pc;
    vm->pc = opcode & 0x0FFFu;
    return 0;
reg_eq_im:
    // 3XNN: Skip next instruction if Vx==00NN
    vm->pc += (VX == (opcode & 0xFF)) ? 4 : 2;
    return 0;
reg_neq_im:
    // 4XNN: Skip next instruction if Vx!=00NN
    vm->pc += (VX == (opcode & 0xFF)) ? 2 : 4;
    return 0;
reg_eq_reg:
    // 5XY0: Skip next instruction if Vx==Vy
    vm->pc += (VX == VY) ? 4 : 2;
    return 0;
load_halfword:
    // 6XNN: Set Vx to 00NN.
    VX = (uint8_t)opcode & 0x00FFu;
    vm->pc += 2;
    return 0;
add_halfword:
    // 7XNN: Add 00NN to Vx without affecting carry.
    VX += (uint8_t)opcode & 0x00FFu;
    vm->pc += 2;
    return 0;
math : {
    int ret_val = 0;
    switch (opcode & 0x000Fu) {
    case 0x0:
        // 8XY0: Vx = Vy
        VX = VY;
        break;
    case 0x1:
        // 8XY1: Vx = Vx | Vy
        VX |= VY;
        break;
    case 0x2:
        // 8XY2: Vx = Vx & Vy
        VX &= VY;
        break;
    case 0x3:
        // 8XY3: Vx = Vx ^ Vy
        VX ^= VY;
        break;
    case 0x4:
        // 8XY4: Vx+=Vy, carry-aware
        vm->v[0xF] = ((uint32_t)VX + VY) > UINT8_MAX;
        VX += VY;
        break;
    case 0x5:
        // 8XY5: Borrow-aware sub. NB: VF set to !borrow
        vm->v[0xF] = VX > VY;
        VX -= VY;
        break;
    case 0x6:
        // 8XY6: Euclidean division by two; set VF if remainder
        // I.e. arithmetic right shift, storing LSB in VF
        vm->v[0xF] = VX & 1;
        VX >>= 1;
        break;
    case 0x7:
        // 8XY7: same as 0x5, but Vy-Vx instead of Vx-Vy
        vm->v[0xF] = VY > VX;
        VY -= VX;
        break;
    case 0xE:
        // 8XYE: Multiplication by two; set VF if remainder
        // I.e. arithmetic left shift, storing MSB in VF
        vm->v[0xF] = VX & 0x80;
        VX <<= 1;
        break;
    default:
        set_error("Unknown opcode " OP_FMT, opcode);
        ret_val = 1;
    }
    vm->pc += 2;
    return ret_val;
}
reg_neq_reg:
    // 9XY0: Skip if Vx!=Vy
    vm->pc += (VX != VY) ? 4 : 2;
    return 0;
set_idx:
    // ANNN
    vm->idx = opcode & 0x0FFF;
    vm->pc += 2;
    return 0;
jump_idx_plus_reg:
    // BNNN
    vm->pc = vm->v[0] + (opcode & 0x0FFF);
    return 0;
random_and:
    // CXNN: Set Vx to NN & random number 0-225
    VX = (opcode & 0x00FF) & (rand() % 255);
    vm->pc += 2;
    return 0;
draw:
    draw_sprite(vm, opcode);
    vm->pc += 2;
    return 0;
exxx_keyops:
    switch (opcode & 0x00FF) {
    case 0x9E:
        // EX9E: Skip next instruction if key stored in Vx pressed
        vm->pc += vm->keyboard[(VX)] ? 4 : 2;
        return 0;
    case 0xA1:
        // EX9E: Skip next instruction if key stored in Vx NOT pressed
        vm->pc += vm->keyboard[(VX)] ? 2 : 4;
        return 0;
    default:
        set_error("Unknown opcode " OP_FMT, opcode);
        return 1;
    }
fxxx_ops:
    switch (opcode & 0x00FF) {
    case 0x07:
        // FX07: Set Vx to value of delay timer
        VX = vm->delay_timer;
        vm->pc += 2;
        break;
    case 0x0A:
        // FX0A: Blocking I/O
        // Frames continue, but the pc doesn't advance until
        // a key is pressed
        if (!key_pressed)
            return 0;
        for (size_t i = 0; i < LEN(vm->keyboard); i++) {
            if (vm->keyboard[i]) {
                VX = i;
                vm->pc += 2;
                return 0;
            }
        }
        break;
    case 0x15:
        // FX15: Set delay timer to the value of Vx
        vm->delay_timer = VX;
        vm->pc += 2;
        break;
    case 0x18:
        // FX18: Set sound timer to the value of Vx
        vm->sound_timer = VX;
        vm->pc += 2;
        break;
    case 0x1E:
        // FX1E: Add Vx to idx. Set 0xF if there is overflow.
        vm->v[0xF] = (((uint32_t)VX + vm->idx) > UINT16_MAX) ? 1 : 0;
        vm->idx += VX;
        vm->pc += 2;
        break;
    case 0x29:
        // FX29: Set idx to location char in Vx's sprite
        vm->idx = VX * 5u;
        vm->pc += 2;
        break;
    case 0x33:
        // FX33: Store BCD representation of Vx in idx through idx+3
        vm->mem[vm->idx] = VX / 100;
        vm->mem[vm->idx + 1] = (VX / 10) % 10;
        vm->mem[vm->idx + 2] = VX % 10;
        vm->pc += 2;
        break;
    case 0x55:
        // FX55: Register dump. Stores V0 through VX in memory,
        // starting at idx
        for (size_t i = 0; i <= ((opcode & 0x0F00u) >> 8u); i++) {
            vm->mem[(vm->idx) + i] = vm->v[i];
        }
        vm->pc += 2;
        break;
    case 0x65:
        // FX55: Register load. Loads V0 through VX from memory,
        // starting from idx
        for (size_t i = 0; i <= ((opcode & 0x0F00u) >> 8u); i++) {
            vm->v[i] = vm->mem[(vm->idx) + i];
        }
        vm->pc += 2;
        break;
    default:
        set_error("Unknown opcode " OP_FMT, opcode);
        return 1;
    }
    return 0;
}
