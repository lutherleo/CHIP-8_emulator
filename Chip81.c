#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>

#define SCALE 10  // Each CHIP-8 pixel will be 10x10 screen pixels
#define WINDOW_WIDTH (DISPLAY_WIDTH * SCALE)
#define WINDOW_HEIGHT (DISPLAY_HEIGHT * SCALE)

// CHIP-8 has a 64x32 monochrome display
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

// Font set - each character is 5 bytes
// These are the built-in sprites for hexadecimal digits 0-F
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

typedef struct {
    uint8_t memory[4096];           // 4KB of RAM
    uint8_t V[16];                  // 16 general purpose 8-bit registers (V0-VF)
    uint16_t I;                     // 16-bit index register
    uint16_t pc;                    // Program counter
    uint8_t display[DISPLAY_WIDTH * DISPLAY_HEIGHT]; // Display pixels
    uint8_t delay_timer;            // Delay timer
    uint8_t sound_timer;            // Sound timer
    uint16_t stack[16];             // Stack for subroutines
    uint8_t sp;                     // Stack pointer
    uint8_t keypad[16];             // Keypad state (0-F)
} Chip8;

//SDL
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} SDLContext;

//called a prototype so it is called sooner.
void chip8_execute(Chip8 *chip8, uint16_t opcode);
int sdl_init(SDLContext *sdl);
void sdl_cleanup(SDLContext *sdl);
void chip8_render(Chip8 *chip8, SDLContext *sdl);

// Initialize the CHIP-8 system
void chip8_init(Chip8 *chip8) {
    // Clear everything
    memset(chip8, 0, sizeof(Chip8));
    
    // Program counter starts at 0x200
    chip8->pc = 0x200;
    
    // Load fontset into memory (starts at 0x000)
    memcpy(chip8->memory, chip8_fontset, 80);
    
    // Seed random number generator
    srand(time(NULL));
}

// Load a program into memory
int chip8_load_rom(Chip8 *chip8, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return 0;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    
    // Check if ROM fits in memory
    if (file_size > (4096 - 0x200)) {
        printf("Error: ROM too large\n");
        fclose(file);
        return 0;
    }
    
    // Load ROM into memory starting at 0x200
    fread(chip8->memory + 0x200, 1, file_size, file);
    fclose(file);
    
    printf("Loaded ROM: %ld bytes\n", file_size);
    return 1;
}

// Fetch the next instruction (2 bytes)
uint16_t chip8_fetch(Chip8 *chip8) {
    // CHIP-8 instructions are 2 bytes, stored big-endian
    // Combine two consecutive bytes into one 16-bit instruction
    uint16_t opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc + 1];
    chip8->pc += 2;
    return opcode;
}

// Execute one cycle
void chip8_cycle(Chip8 *chip8) {
    // Fetch instruction
    uint16_t opcode = chip8_fetch(chip8);
    
    // Execute instruction
    chip8_execute(chip8, opcode);
    
    // Update timers
    if (chip8->delay_timer > 0) {
        chip8->delay_timer--;
    }
    
    if (chip8->sound_timer > 0) {
        chip8->sound_timer--;
    }
}
//Clear display function
void chip8_execute(Chip8 *chip8, uint16_t opcode) {
    if (opcode == 0x00E0) {
        memset(chip8->display, 0, sizeof(chip8->display));
    }
    else if ((opcode & 0xF000) == 0x6000) {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t nn = opcode & 0x00FF;
        chip8->V[x] = nn; 
    }
    else if ((opcode & 0xF000) == 0x7000) {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t nn = opcode & 0x00FF;
        chip8->V[x] += nn;
    }
    else if ((opcode & 0xF000) == 0x1000) {
        uint16_t nnn = opcode & 0x0FFF;
        chip8->pc = nnn; //we incremented pc by 2 in fetch, so we jump by setting nnn.
    }
    else if ((opcode & 0xF000) == 0x2000) { // to jump to subroutine
        uint16_t nnn = opcode & 0x0FFF;
        chip8->stack[chip8->sp] = chip8->pc;
        chip8->sp++;
        chip8->pc = nnn;
    }
    else if (opcode == 0x00EE) {
        chip8->sp--;
        chip8->pc = chip8->stack[chip8->sp];
    }
    else if ((opcode & 0xF000) == 0x3000) { //skipping
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t nn = opcode & 0x00FF;
        if (chip8->V[x]  == nn) {
            chip8->pc += 2;             
        }
    }
    else if ((opcode & 0xF000) == 0x4000) {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t nn = opcode & 0x00FF;
        if (chip8->V[x] != nn) {
            chip8->pc += 2;             
        }
    }
    else if ((opcode & 0xF00F) == 0x5000) {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t y = (opcode & 0x00F0) >> 4;
        if (chip8->V[x] == chip8->V[y]) {
            chip8->pc += 2;             
        }
    }
    else if ((opcode & 0xF00F) == 0x9000) {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t y = (opcode & 0x00F0) >> 4;
        if (chip8->V[x] != chip8->V[y]) {
            chip8->pc += 2;             
        }
    }
    else if((opcode & 0xF000) == 0x8000) {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t y = (opcode & 0x00F0) >> 4;
        uint8_t z = opcode & 0x000F;
        
        switch (z) {
            case 0:
                chip8->V[x] = chip8->V[y];
            break;
            case 1:
                chip8->V[x] = (chip8->V[x]) | (chip8->V[y]);
            break;
            case 2:
                chip8->V[x] = (chip8->V[x]) & (chip8->V[y]);
            break;
            case 3:
                chip8->V[x] = (chip8->V[x]) ^ (chip8->V[y]);
            break;
            case 4:
                uint16_t sum = chip8->V[x] + chip8->V[y];  
                chip8->V[0xF] = (sum > 255) ? 1 : 0;      
                chip8->V[x] = sum & 0xFF; 
            break;
            case 5:
            /*uint8_t sub;    
            if(chip8->V[x] < chip8->V[y]){
                    sub = chip8->V[x] - chip8->V[y] + 0xFF;
                    chip8->V[0xF] = 0x0;
                }
            else if(chip8->V[x] > chip8->V[y]){
                    sub = chip8->V[x] - chip8->V[y];
                    chip8->V[0xF] = 0x1;
            } */
            chip8->V[0xF] = (chip8->V[x] >= chip8->V[y]) ? 1 : 0;
            chip8->V[x] = chip8->V[x] - chip8->V[y];
            break;
            case 6:
                chip8->V[0xF] = chip8->V[x] & 0x01;
                chip8->V[x] = chip8->V[x]>>1;
            break;
            case 7:
            /*uint8_t sub;    
            if(chip8->V[y] < chip8->V[x]){
                    sub = chip8->V[y] - chip8->V[x] + 0xFF;
                    chip8->V[0xF] = 0x0;
                }
            else if(chip8->V[y] > chip8->V[x]){
                    sub = chip8->V[y] - chip8->V[x];
                    chip8->V[0xF] = 0x1;
            } */   
            chip8->V[0xF] = (chip8->V[y] >= chip8->V[x]) ? 1 : 0;  
            chip8->V[x] = chip8->V[y] - chip8->V[x];
            break;
        }
    }
    else if((opcode & 0xF000) == 0xA000) {
        uint16_t nnn = opcode & 0x0FFF;
        chip8->I = nnn;
    }
    else if((opcode & 0xF000) == 0xB000) {
        uint16_t nnn = opcode & 0x0FFF;
        chip8->pc = nnn + chip8->V[0];
    }
    else if((opcode & 0xF000) == 0xC000) {
        uint8_t nn = opcode & 0x00FF;
        uint8_t x = (opcode & 0x0F00) >> 8;
        chip8->V[x] = (rand()%255) & nn; 
    }
    else if((opcode & 0xF000) == 0xF000) {
        uint8_t nn = opcode & 0x00FF;
        uint8_t x = (opcode & 0x0F00) >> 8;
        int i;
        int key_pressed;
        switch(nn) {
            case 0x7:
                chip8->V[x] = chip8->delay_timer;
                break;
            case 0x15:
                chip8->delay_timer = chip8->V[x];
                break;
            case 0x18:
                chip8->sound_timer = chip8->V[x];
                break;
            case 0x1E:
                chip8->I += chip8->V[x];
                break;
            case 0x29:
                chip8->I = chip8->V[x] * 5; //cuz character is 5 bytes tall
                break;
            case 0x33:
                chip8->memory[chip8->I] = chip8->V[x]/100;       
                chip8->memory[chip8->I +1] = (chip8->V[x]/10)%10;
                chip8->memory[chip8->I +2] = chip8->V[x]%10;
                break;
            case 0x55:
                i = 0;
                while(i<=x){
                    chip8->memory[chip8->I + i] = chip8->V[i];
                    i++;
                }
                break;
            case 0x65:
                i = 0;
                while(i<=x){
                    chip8->V[i] = chip8->memory[chip8->I + i]; 
                    i++;
                }
                break;
            case 0x0A: // i dont get this
                // Check if any key is pressed
                int key_pressed = -1;  // -1 means no key pressed yet
                for (i = 0; i < 16; i++) {
                    if (chip8->keypad[i]) {
                        key_pressed = i;
                        break;
                    }
                }

                if (key_pressed != -1) {
                    // A key was pressed! Store it and continue
                    chip8->V[x] = key_pressed;
                } else {
                    // No key pressed yet - repeat this instruction
                    chip8->pc -= 2;  // Go back 2 bytes to re-execute this opcode
                }
                break;
        }

    }
    else if ((opcode & 0xF0FF) == 0xE09E) {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t key = chip8->V[x];
        if (chip8->keypad[key]) {
            chip8->pc += 2; //skip next instruction
        }
    }
    else if ((opcode & 0xF0FF) == 0xE0A1) {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t key = chip8->V[x];
        if (!chip8->keypad[key]) {
            chip8->pc += 2; //skip next instruction
        }
    }
    else if ((opcode & 0xF000) == 0xD000){
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t y = (opcode & 0x00F0) >> 4;
        uint8_t n = (opcode & 0x000F);
        chip8->V[0xF] = 0;
        int i,sprite_byte;
        for(i=0;i < n; i++){
            sprite_byte = chip8->memory[chip8->I + i];
            for(int j=0; j < 8; j++){
                if (sprite_byte & (0x80 >> j)){
                    uint8_t x_pos = (chip8->V[x] + j) % 64;
                    uint8_t y_pos = (chip8->V[y] + i) % 32;

                    int pixel_index = y_pos * 64 + x_pos;

                    if (chip8->display[pixel_index] == 1) {
                        chip8->V[0xF] = 1;  // Collision detected!
                    }

                    chip8->display[pixel_index] ^= 1;
                }
            }
        } 
    }

}

int sdl_init(SDLContext *sdl) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return 0; 
    }
    // Create window
    sdl->window = SDL_CreateWindow(
    "CHIP-8 Emulator",           // Window title
    SDL_WINDOWPOS_CENTERED,      // X position (centered)
    SDL_WINDOWPOS_CENTERED,      // Y position (centered)
    WINDOW_WIDTH,                // Width
    WINDOW_HEIGHT,               // Height
    SDL_WINDOW_SHOWN             // Flags
    );

    if (!sdl->window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }
    // Create renderer
    sdl->renderer = SDL_CreateRenderer(
    sdl->window,                 // The window to render to
    -1,                          // Driver index (-1 = first available)
    SDL_RENDERER_ACCELERATED     // Use hardware acceleration
    );

    if (!sdl->renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdl->window);
        SDL_Quit();
        return 0;
    }
    // Create texture for the display
    sdl->texture = SDL_CreateTexture(
    sdl->renderer,
    SDL_PIXELFORMAT_RGBA8888,    // Pixel format (32-bit RGBA)
    SDL_TEXTUREACCESS_STREAMING, // We'll update it every frame
    DISPLAY_WIDTH,               // 64 pixels wide
    DISPLAY_HEIGHT               // 32 pixels tall
    );

    if (!sdl->texture) {
        printf("Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(sdl->renderer);
        SDL_DestroyWindow(sdl->window);
        SDL_Quit();
        return 0;
    }
    return 1;
}

void sdl_cleanup(SDLContext *sdl) {
    SDL_DestroyTexture(sdl->texture);
    SDL_DestroyRenderer(sdl->renderer);
    SDL_DestroyWindow(sdl->window);
    SDL_Quit();
}

void chip8_render(Chip8 *chip8, SDLContext *sdl) {
    // Create a pixel buffer (RGBA format - 32 bits per pixel)
    uint32_t pixels[DISPLAY_WIDTH * DISPLAY_HEIGHT];
    
    // Convert CHIP-8 display (1 bit per pixel) to RGBA pixels
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        // White if pixel is on (1), black if off (0)
        pixels[i] = chip8->display[i] ? 0xFFFFFFFF : 0x00000000;
    }
    
    // Update the texture with our pixel data
    SDL_UpdateTexture(sdl->texture, NULL, pixels, DISPLAY_WIDTH * sizeof(uint32_t));
    
    // Clear the renderer
    SDL_RenderClear(sdl->renderer);
    
    // Copy texture to renderer (scales it to window size)
    SDL_RenderCopy(sdl->renderer, sdl->texture, NULL, NULL);
    
    // Present the rendered frame
    SDL_RenderPresent(sdl->renderer);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <ROM file>\n", argv[0]);
        return 1;
    }
    
    Chip8 chip8;
    SDLContext sdl;
    
    chip8_init(&chip8);
    
    if (!sdl_init(&sdl)) {
        return 1;
    }
    
    if (!chip8_load_rom(&chip8, argv[1])) {
        sdl_cleanup(&sdl);
        return 1;
    }
    
    // Main emulation loop
    int quit = 0;
    SDL_Event event;
    
    // Timing variables
    const int FPS = 60;
    const int FRAME_DELAY = 1000 / FPS;  // milliseconds per frame
    uint32_t frame_start;
    int frame_time;
    
    while (!quit) {
        frame_start = SDL_GetTicks();
        
        // Handle input events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
            else if (event.type == SDL_KEYDOWN) {
                // Map keyboard keys to CHIP-8 keypad
                switch (event.key.keysym.sym) {
                    case SDLK_1: chip8.keypad[0x1] = 1; break;
                    case SDLK_2: chip8.keypad[0x2] = 1; break;
                    case SDLK_3: chip8.keypad[0x3] = 1; break;
                    case SDLK_4: chip8.keypad[0xC] = 1; break;
                    
                    case SDLK_q: chip8.keypad[0x4] = 1; break;
                    case SDLK_w: chip8.keypad[0x5] = 1; break;
                    case SDLK_e: chip8.keypad[0x6] = 1; break;
                    case SDLK_r: chip8.keypad[0xD] = 1; break;
                    
                    case SDLK_a: chip8.keypad[0x7] = 1; break;
                    case SDLK_s: chip8.keypad[0x8] = 1; break;
                    case SDLK_d: chip8.keypad[0x9] = 1; break;
                    case SDLK_f: chip8.keypad[0xE] = 1; break;
                    
                    case SDLK_z: chip8.keypad[0xA] = 1; break;
                    case SDLK_x: chip8.keypad[0x0] = 1; break;
                    case SDLK_c: chip8.keypad[0xB] = 1; break;
                    case SDLK_v: chip8.keypad[0xF] = 1; break;
                    
                    case SDLK_ESCAPE: quit = 1; break;
                }
            }
            else if (event.type == SDL_KEYUP) {
                // Release keys
                switch (event.key.keysym.sym) {
                    case SDLK_1: chip8.keypad[0x1] = 0; break;
                    case SDLK_2: chip8.keypad[0x2] = 0; break;
                    case SDLK_3: chip8.keypad[0x3] = 0; break;
                    case SDLK_4: chip8.keypad[0xC] = 0; break;
                    
                    case SDLK_q: chip8.keypad[0x4] = 0; break;
                    case SDLK_w: chip8.keypad[0x5] = 0; break;
                    case SDLK_e: chip8.keypad[0x6] = 0; break;
                    case SDLK_r: chip8.keypad[0xD] = 0; break;
                    
                    case SDLK_a: chip8.keypad[0x7] = 0; break;
                    case SDLK_s: chip8.keypad[0x8] = 0; break;
                    case SDLK_d: chip8.keypad[0x9] = 0; break;
                    case SDLK_f: chip8.keypad[0xE] = 0; break;
                    
                    case SDLK_z: chip8.keypad[0xA] = 0; break;
                    case SDLK_x: chip8.keypad[0x0] = 0; break;
                    case SDLK_c: chip8.keypad[0xB] = 0; break;
                    case SDLK_v: chip8.keypad[0xF] = 0; break;
                }
            }
        }
        
        // Execute several CPU cycles per frame (adjust for speed)
        for (int i = 0; i < 10; i++) {
            chip8_cycle(&chip8);
        }
        
        // Render the display
        chip8_render(&chip8, &sdl);
        
        // Cap frame rate
        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }
    
    sdl_cleanup(&sdl);
    return 0;
}