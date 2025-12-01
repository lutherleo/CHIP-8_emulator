#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

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

//called a prototype so it is called sooner.
void chip8_execute(Chip8 *chip8, uint16_t opcode);

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
    
    // Decode and execute instruction
    // We'll implement this next!
    printf("Opcode: 0x%04X\n", opcode);
    
    // Update timers
    if (chip8->delay_timer > 0) {
        chip8->delay_timer--;
    }
    
    if (chip8->sound_timer > 0) {
        if (chip8->sound_timer == 1) {
            printf("BEEP!\n");
        }
        chip8->sound_timer--;
    }
    chip8_execute(chip8, opcode);
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
            /*case 0x55:
                uint8_t i = 0;
                while(i<=x){
                    chip8->memory[chip8->I + i] =
                }
                break;
            case 0x65:

                break;
            case 0x0A:

                break;
        }*/

    }

}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <ROM file>\n", argv[0]);
        return 1;
    }
    
    Chip8 chip8;
    chip8_init(&chip8);
    
    if (!chip8_load_rom(&chip8, argv[1])) {
        return 1;
    }
    
    // Run a few cycles for testing
    for (int i = 0; i < 10; i++) {
        chip8_cycle(&chip8);
    }
    
    return 0;
}