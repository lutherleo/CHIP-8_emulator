# CHIP-8 Emulator
Emulator for CHIP-8 games written in C

A fully functional CHIP-8 interpreter that can run classic CHIP-8 games and programs.

## Features

- Complete implementation of all 35 CHIP-8 opcodes
- 4KB RAM with proper memory mapping
- 16 general-purpose 8-bit registers (V0-VF)
- Stack support for subroutines (16 levels)
- 64x32 monochrome display with XOR sprite drawing
- Delay and sound timers
- 16-key hexadecimal keypad input
- SDL2-based graphics rendering with 10x scaling
- 60 FPS frame rate control

## Requirements

- GCC compiler
- SDL2 development libraries

### Installing SDL2

**Ubuntu/Debian:**
```bash
sudo apt-get install libsdl2-dev
```

**macOS (with Homebrew):**
```bash
brew install sdl2
```

**Fedora:**
```bash
sudo dnf install SDL2-devel
```

## Compilation

Compile using:
```bash
gcc chip8.c -o chip8 -lSDL2
```

## Usage

Run the emulator with a CHIP-8 ROM:
```bash
./chip8 <rom_file.ch8>
```

Example with included ROM:
```bash
./chip8 Nim.ch8
```

## Controls

The CHIP-8 has a 16-key hexadecimal keypad (0-F) which is mapped to your keyboard:

```
CHIP-8 Keypad:          Keyboard Mapping:
┌───┬───┬───┬───┐       ┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ C │       │ 1 │ 2 │ 3 │ 4 │
├───┼───┼───┼───┤       ├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ D │       │ Q │ W │ E │ R │
├───┼───┼───┼───┤  →    ├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ E │       │ A │ S │ D │ F │
├───┼───┼───┼───┤       ├───┼───┼───┼───┤
│ A │ 0 │ B │ F │       │ Z │ X │ C │ V │
└───┴───┴───┴───┘       └───┴───┴───┴───┘
```

Press ESC to quit the emulator.

## Included ROM

I've added Nim - a mathematical strategy game where players take turns removing objects from distinct heaps.

## Technical Details

### Memory Map
- `0x000-0x1FF`: Reserved for interpreter (font data)
- `0x200-0xFFF`: Program ROM and work RAM

### CPU Specifications
- 16-bit address space (4KB)
- 16 8-bit general-purpose registers (V0-VF)
- 16-bit index register (I)
- 16-bit program counter (PC)
- 8-bit stack pointer (SP)
- 16-level stack for subroutines

### Display
- 64x32 pixels, monochrome
- Sprites drawn using XOR mode
- Built-in font sprites for hexadecimal digits (0-F)

### Timers
- Delay timer: Decrements at 60 Hz
- Sound timer: Decrements at 60 Hz, beeps when non-zero

### Performance
- Runs at 10 instructions per frame (600 instructions/second)
- Adjustable by changing the loop count in main()

## Implemented Opcodes

All 35 CHIP-8 opcodes are fully implemented:
- `0x00E0` - Clear display
- `0x00EE` - Return from subroutine
- `0x1NNN` - Jump to address
- `0x2NNN` - Call subroutine
- `0x3XNN` - Skip if VX == NN
- `0x4XNN` - Skip if VX != NN
- `0x5XY0` - Skip if VX == VY
- `0x6XNN` - Set VX = NN
- `0x7XNN` - Add NN to VX
- `0x8XY0` - Set VX = VY
- `0x8XY1` - Set VX = VX OR VY
- `0x8XY2` - Set VX = VX AND VY
- `0x8XY3` - Set VX = VX XOR VY
- `0x8XY4` - Add VY to VX (with carry)
- `0x8XY5` - Subtract VY from VX (with borrow)
- `0x8XY6` - Shift VX right
- `0x8XY7` - Set VX = VY - VX (with borrow)
- `0x9XY0` - Skip if VX != VY
- `0xANNN` - Set I = NNN
- `0xBNNN` - Jump to NNN + V0
- `0xCXNN` - Set VX = random & NN
- `0xDXYN` - Draw sprite
- `0xEX9E` - Skip if key VX is pressed
- `0xEXA1` - Skip if key VX is not pressed
- `0xFX07` - Set VX = delay timer
- `0xFX0A` - Wait for key press
- `0xFX15` - Set delay timer = VX
- `0xFX18` - Set sound timer = VX
- `0xFX1E` - Add VX to I
- `0xFX29` - Set I = location of sprite for digit VX
- `0xFX33` - Store BCD representation of VX
- `0xFX55` - Store V0-VX in memory starting at I
- `0xFX65` - Load V0-VX from memory starting at I

## Finding ROMs

CHIP-8 ROMs available at: https://github.com/kripod/chip8-roms

## Troubleshooting

**SDL initialization failure**
- Ensure SDL2 is properly installed
- Reinstall SDL2 development libraries if needed

**ROM fails to load**
- Verify the ROM file exists and is readable
- CHIP-8 ROMs should be less than 3.5KB

**Game runs too fast or too slow**
- Adjust the instruction count in the main loop
- Increase the number for faster execution, decrease for slower