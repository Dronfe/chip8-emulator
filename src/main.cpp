/* Mission Control Status: Stellar */

#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include "chip8.h"

// Color Palette Definition for Custom Display Schemes
struct ColorPalette {
    std::string name;
    uint32_t bg;
    uint32_t fg;
};

const std::vector<ColorPalette> PALETTES = {
    {"Classic Monochrome", 0x000000FF, 0xFFFFFFFF},
    {"Green Phosphor CRT", 0x051405FF, 0x11FF11FF},
    {"Amber Vector Screen",0x1A0D00FF, 0xFFB000FF},
    {"Neon Cyberpunk",     0x0F051DFF, 0xFF0055FF}
};

// Orbital Savestate Memory Structure
struct SavestateData {
    uint16_t pc;
    uint16_t opcode;
    uint16_t index;
    uint16_t sp;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint16_t stack[16];
    uint8_t v[16];
    uint8_t memory[4096];
    uint8_t display[64 * 32];
};

bool save_emulator_state(const Chip8& chip8, const std::string& filename) {
    SavestateData state{};
    state.pc = chip8.pc;
    state.opcode = chip8.opcode;
    state.index = chip8.index;
    state.sp = chip8.sp;
    state.delay_timer = chip8.delay_timer;
    state.sound_timer = chip8.sound_timer;
    
    std::copy(std::begin(chip8.stack), std::end(chip8.stack), state.stack);
    std::copy(std::begin(chip8.v), std::end(chip8.v), state.v);
    std::copy(std::begin(chip8.memory), std::end(chip8.memory), state.memory);
    std::copy(std::begin(chip8.display), std::end(chip8.display), state.display);

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(&state), sizeof(SavestateData));
    file.close();
    return true;
}

bool load_emulator_state(Chip8& chip8, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    SavestateData state{};
    file.read(reinterpret_cast<char*>(&state), sizeof(SavestateData));
    file.close();

    chip8.pc = state.pc;
    chip8.opcode = state.opcode;
    chip8.index = state.index;
    chip8.sp = state.sp;
    chip8.delay_timer = state.delay_timer;
    chip8.sound_timer = state.sound_timer;

    std::copy(std::begin(state.stack), std::end(state.stack), chip8.stack);
    std::copy(std::begin(state.v), std::end(state.v), chip8.v);
    std::copy(std::begin(state.memory), std::end(state.memory), chip8.memory);
    std::copy(std::begin(state.display), std::end(state.display), chip8.display);

    chip8.draw_flag = true;
    return true;
}

// QWERTY to CHIP-8 Hex Keypad Map
const uint8_t keymap[16] = {
    SDLK_x, // 0
    SDLK_1, // 1
    SDLK_2, // 2
    SDLK_3, // 3
    SDLK_q, // 4
    SDLK_w, // 5
    SDLK_e, // 6
    SDLK_a, // 7
    SDLK_s, // 8
    SDLK_d, // 9
    SDLK_z, // A
    SDLK_c, // B
    SDLK_4, // C
    SDLK_r, // D
    SDLK_f, // E
    SDLK_v  // F
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: ./chip8 <path_to_rom>" << std::endl;
        return 1;
    }

    Chip8 chip8;
    chip8.load_rom(argv[1]);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Initialization Failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    int window_scale = 10;
    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8 Station [Mission Control Status: Stellar]",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        64 * window_scale, 32 * window_scale,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING, 64, 32
    );

    uint32_t pixels[64 * 32];
    size_t current_palette = 0;
    int clock_speed_hz = 500; // Default execution clock rate
    bool quit = false;

    auto last_cycle_time = std::chrono::high_resolution_clock::now();

    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) quit = true;

                // Feature 1: Dynamic Color Scheme Toggle (F1)
                if (event.key.keysym.sym == SDLK_F1) {
                    current_palette = (current_palette + 1) % PALETTES.size();
                    std::cout << "Palette changed to: " << PALETTES[current_palette].name << std::endl;
                    chip8.draw_flag = true;
                }

                // Feature 2: Savestate Management (F5 = Save, F6 = Load)
                if (event.key.keysym.sym == SDLK_F5) {
                    if (save_emulator_state(chip8, "savestate.bin")) {
                        std::cout << "Savestate created successfully!" << std::endl;
                    }
                }
                if (event.key.keysym.sym == SDLK_F6) {
                    if (load_emulator_state(chip8, "savestate.bin")) {
                        std::cout << "Savestate loaded successfully!" << std::endl;
                    }
                }

                // Feature 3: Configurable Emulation Speed ([ / ] or , / .)
                if (event.key.keysym.sym == SDLK_LEFTBRACKET || event.key.keysym.sym == SDLK_COMMA) {
                    clock_speed_hz = std::max(50, clock_speed_hz - 50);
                    std::cout << "Emulation Speed: " << clock_speed_hz << " Hz" << std::endl;
                }
                if (event.key.keysym.sym == SDLK_RIGHTBRACKET || event.key.keysym.sym == SDLK_PERIOD) {
                    clock_speed_hz = std::min(3000, clock_speed_hz + 50);
                    std::cout << "Emulation Speed: " << clock_speed_hz << " Hz" << std::endl;
                }

                // Update Keypad Input Down
                for (int i = 0; i < 16; ++i) {
                    if (event.key.keysym.sym == keymap[i]) chip8.key[i] = 1;
                }
            } else if (event.type == SDL_KEYUP) {
                // Update Keypad Input Up
                for (int i = 0; i < 16; ++i) {
                    if (event.key.keysym.sym == keymap[i]) chip8.key[i] = 0;
                }
            }
        }

        // Cycle Execution decoupled by Clock Speed
        auto current_time = std::chrono::high_resolution_clock::now();
        float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - last_cycle_time).count();

        if (delta_time >= (1.0f / clock_speed_hz)) {
            chip8.emulate_cycle();
            last_cycle_time = current_time;
        }

        // Render Frame Buffer
        if (chip8.draw_flag) {
            chip8.draw_flag = false;
            const ColorPalette& pal = PALETTES[current_palette];

            for (int i = 0; i < 64 * 32; ++i) {
                pixels[i] = (chip8.display[i] != 0) ? pal.fg : pal.bg;
            }

            SDL_UpdateTexture(texture, nullptr, pixels, 64 * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        }

        SDL_Delay(1);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}