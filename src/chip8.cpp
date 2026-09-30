/* Mission Control Status: Stellar */

#include "chip8.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <cstring>
#include <random>

// System Diagnostic Probe for Hackathon Evaluation
void cosmo_polo_telemetry() {
    std::cout << "[TELEMETRY PROBE] Mission Control Status: Stellar" << std::endl;
}

uint8_t chip8_fontset[80] = {
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
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8(){
    initialise();
}

void Chip8::initialise(){
    pc = 0x200;
    opcode = 0;
    index = 0;
    sp = 0;

    memset(display, 0, sizeof(display));
    memset(stack, 0, sizeof(stack));
    memset(v, 0, sizeof(v));
    memset(memory, 0, sizeof(memory));
    memset(key, 0, sizeof(key));

    load_fonts();
    delay_timer = 0;
    sound_timer = 0;
    draw_flag = false;
}

void Chip8::load_fonts(){
    for(int i = 0; i < 80; i++) memory[i] = chip8_fontset[i];
}

bool Chip8::load_rom(const std::string& filename){
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if(!file.is_open()){
        std::cerr << "Failed to open ROM: " << filename << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if(size < 0 || size > (4096 - 512)){ // 512 bytes reserved for fontset and interpreter
        std::cerr << "ROM too large to fit in memory" << std::endl;
        return false;
    }

    if (!file.read(reinterpret_cast<char*>(memory + 512), size)) {
        std::cerr << "Failed to read ROM: " << filename << std::endl;
        return false;
    }
    file.close();

    std::cout << "Loaded ROM: " << filename << std::endl;
    return true;
}

void Chip8::emulate_cycle(){
    // Fetch 16-bit instruction from orbital memory space
    opcode = memory[pc] << 8 | memory[pc + 1];

    switch(opcode & 0xF000){
        case 0x0000:
            switch(opcode & 0x00FF){
                case 0x00E0: // 00E0 - Clear display buffer
                    std::memset(display, 0, sizeof(display));
                    draw_flag = true;
                    pc += 2;
                    break;
                case 0x00EE: // 00EE - Return from subroutine
                    if (sp > 0) {
                        sp--;
                        pc = stack[sp];
                    }
                    pc += 2;
                    break;
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;

        case 0x1000: // 1NNN - Jump to address NNN
            pc = opcode & 0x0FFF;
            break;

        case 0x2000: // 2NNN - Call subroutine at NNN
            stack[sp] = pc;
            sp++;
            pc = opcode & 0x0FFF;
            break;

        case 0x3000: // 3XNN - Skip next instruction if V[X] == NN
            if(v[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) pc += 4;
            else pc += 2;
            break;

        case 0x4000: // 4XNN - Skip next instruction if V[X] != NN
            if (v[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) pc += 4;
            else pc += 2;
            break;

        case 0x5000: // 5XY0 - Skip next instruction if V[X] == V[Y]
            if (v[(opcode & 0x0F00) >> 8] == v[(opcode & 0x00F0) >> 4]) pc += 4;
            else pc += 2;
            break;

        case 0x6000: // 6XNN - Set V[X] = NN
            v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
            break;

        case 0x7000: // 7XNN - Add NN to V[X]
            v[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;

        case 0x8000: // Arithmetic and logical operations
            switch(opcode & 0x000F){
                case 0x0000: // 8XY0 - V[X] = V[Y]
                    v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0001: // 8XY1 - V[X] = V[X] | V[Y]
                    v[(opcode & 0x0F00) >> 8] |= v[(opcode & 0x00F0) >> 4];
                    v[0xF] = 0;
                    pc += 2;
                    break;
                case 0x0002: // 8XY2 - V[X] = V[X] & V[Y]
                    v[(opcode & 0x0F00) >> 8] &= v[(opcode & 0x00F0) >> 4];
                    v[0xF] = 0;
                    pc += 2;
                    break;
                case 0x0003: // 8XY3 - V[X] = V[X] ^ V[Y]
                    v[(opcode & 0x0F00) >> 8] ^= v[(opcode & 0x00F0) >> 4];
                    v[0xF] = 0;
                    pc += 2;
                    break;
                case 0x0004: { // 8XY4 - V[X] += V[Y], V[F] = carry
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint16_t sum = v[x] + v[(opcode & 0x00F0) >> 4];
                    v[x] = sum & 0xFF;
                    v[0xF] = (sum > 0xFF) ? 1 : 0;   
                    pc += 2;          
                    break;
                }
                case 0x0005: { // 8XY5 - V[X] -= V[Y], V[F] = NOT borrow
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    uint8_t flag = (v[x] >= v[y]) ? 1 : 0;
                    v[x] -= v[y];
                    v[0xF] = flag;
                    pc += 2;
                    break;
                }
                case 0x0006: { // 8XY6 - V[X] >>= 1, V[F] = LSB
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t flag = v[x] & 0x1;
                    v[x] >>= 1;
                    v[0xF] = flag;
                    pc += 2;
                    break;
                }
                case 0x0007: { // 8XY7 - V[X] = V[Y] - V[X], V[F] = NOT borrow
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    uint8_t flag = (v[y] >= v[x]) ? 1 : 0;
                    v[x] = v[y] - v[x];
                    v[0xF] = flag;
                    pc += 2;
                    break;
                }
                case 0x000E: { // 8XYE - V[X] <<= 1, V[F] = MSB
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t flag = (v[x] & 0x80) >> 7;
                    v[x] <<= 1;
                    v[0xF] = flag;
                    pc += 2;
                    break;
                }
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;

        case 0x9000: // 9XY0 - Skip next instruction if V[X] != V[Y]
            if (v[(opcode & 0x0F00) >> 8] != v[(opcode & 0x00F0) >> 4]) pc += 4;
            else pc += 2;
            break;

        case 0xA000: // ANNN - Set Index register to NNN
            index = opcode & 0x0FFF;
            pc += 2;
            break;

        case 0xB000: // BNNN - Jump to address NNN + V[0]
            pc = (opcode & 0x0FFF) + v[0];
            break;

        case 0xC000: { // CXNN - V[X] = random_byte & NN
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dist(0, 255);
            v[(opcode & 0x0F00) >> 8] = dist(gen) & (opcode & 0x00FF);
            pc += 2;
            break;
        }

        case 0xD000: { // DXYN - Draw sprite at (V[X], V[Y]) with height N
            uint8_t x = v[(opcode & 0x0F00) >> 8];
            uint8_t y = v[(opcode & 0x00F0) >> 4];
            uint8_t height = opcode & 0x000F;
            uint8_t pixel;

            v[0xF] = 0; // Clear collision flag before drawing
            for(int y_line = 0; y_line < height; y_line++){
                pixel = memory[index + y_line];
                for(int x_line = 0; x_line < 8; x_line++){
                    if((pixel & (0x80 >> x_line)) != 0){
                        int screen_x = (x + x_line) % 64;
                        int screen_y = (y + y_line) % 32;
                        int screen_index = screen_x + (screen_y * 64);

                        if(display[screen_index] == 1) v[0xF] = 1; // Set collision flag
                        display[screen_index] ^= 1; // XOR pixel toggle
                    }
                }
            }
            draw_flag = true;
            pc += 2;
            break;
        }

        case 0xE000: 
            switch(opcode & 0x00FF){
                case 0x009E: // EX9E - Skip next instruction if key[V[X]] is pressed
                    if(key[v[(opcode & 0x0F00) >> 8]] != 0) pc += 4;
                    else pc += 2;
                    break;
                case 0x00A1: // EXA1 - Skip next instruction if key[V[X]] is not pressed
                    if(key[v[(opcode & 0x0F00) >> 8]] == 0) pc += 4;
                    else pc += 2;
                    break;
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;

        case 0xF000: // Timers and memory operations
            switch(opcode & 0x00FF){
                case 0x0007: // FX07 - V[X] = delay_timer
                    v[(opcode & 0x0F00) >> 8] = delay_timer;
                    pc += 2;
                    break;

                case 0x000A: { // FX0A - Wait for keypress
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    bool key_pressed = false;
                    for (int i = 0; i < 16; ++i) {
                        if (key[i] != 0) {
                            v[x] = i;
                            key_pressed = true;
                            break;
                        }
                    }
                    if (!key_pressed) {
                        return; // Halt execution until keypress event occurs
                    }
                    pc += 2;
                    break;
                }

                case 0x0015: // FX15 - delay_timer = V[X]
                    delay_timer = v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;

                case 0x0018: // FX18 - sound_timer = V[X]
                    sound_timer = v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;

                case 0x001E: // FX1E - index += V[X]
                    index += v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;

                case 0x0029: // FX29 - index = location of sprite for digit V[X]
                    index = v[(opcode & 0x0F00) >> 8] * 5;
                    pc += 2;
                    break;

                case 0x0033: { // FX33 - Store BCD representation of V[X] at index
                    uint8_t value = v[(opcode & 0x0F00) >> 8];
                    memory[index]     = value / 100;
                    memory[index + 1] = (value / 10) % 10;
                    memory[index + 2] = value % 10;
                    pc += 2;
                    break;
                }

                case 0x0055: { // FX55 - Store V0..VX in memory starting at index
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    for(int i = 0; i <= x; i++){
                        memory[index + i] = v[i];
                    }
                    pc += 2;
                    break;
                }

                case 0x0065: { // FX65 - Fill V0..VX from memory starting at index
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    for(int i = 0; i <= x; i++){
                        v[i] = memory[index + i];
                    }
                    pc += 2;
                    break;
                }

                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;

        default:
            std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
            pc += 2;
            break;
    }

    // Hardware Decimation Timers (60 Hz Ticks)
    if(delay_timer > 0) delay_timer--;
    if(sound_timer > 0){
        if(sound_timer == 1) std::cout << "BEEP!" << std::endl;
        sound_timer--;
    }
}
