# 48-bit CPU Simulator

A C-based simulator for a hypothetical early computer architecture. This system features a 48-bit word size, tape-based memory, fixed-point arithmetic, and a unique "Overlay/Linker" mechanism for library calls.

## Architecture Overview

- **Word Size**: 48-bit (stored in `int64_t`).
- **Arithmetic**: Fixed-point `[-1, 1)`.
- **Registers**:
  - `R1`, `R2`: Arithmetic registers (can be treated as a single 96-bit register for some operations).
  - `R3`: I/O buffer and multiplication/division helper.
- **Memory**:
  - **Scratchpad Tape**: Main read/write memory (simulated via file).
  - **Library Tape**: Read-only system routines (simulated via file).
- **Input/Output**:
  - **Card Reader**: Input device for loading programs (Bootstrap mode).
  - **Paper Tape**: Output device.

## Features

- **Bootstrap Loading**: Programs are loaded from "cards" using a specific Data/Store sequence.
- **Tape Memory**: Sequential access memory model.
- **Overlay Linking**: The `CALL` instruction dynamically loads functions from the Library Tape into the Scratchpad and links them by rewriting `RET` instructions with `TXR` (Transfer) jumps.
- **Instruction Set**: Includes Load/Store, Arithmetic (Add, Mult, Div, Shift), Logic (And, Or, Xor), and Control Flow (Skip, Transfer).

## Build Instructions

To compile the simulator and utilities:

```bash
# Compile the Simulator
gcc -o cpu cpu.c

# Compile the Utilities
gcc -o card_maker card_maker.c
gcc -o tape_maker tape_maker.c
```

## Usage

### 1. Prepare the Library Tape
Create a source file for your library functions (e.g., `lib_source.txt`) and convert it to a binary tape:

```bash
./tape_maker lib_source.txt library.bin
```

### 2. Prepare the Program Deck
Create a source file for your program (e.g., `test_source.txt`). Note that this file must follow the **Bootstrap Format** (Data Card followed by Instruction Card):

```bash
./card_maker test_source.txt deck.txt
```

### 3. Run the Simulator
Ensure `scratchpad.bin` (will be created), `library.bin`, and `deck.txt` are in the current directory.

```bash
./cpu
```

The output will be written to `output.txt`.

## File Structure

- `cpu.c` / `cpu.h`: Core simulator implementation.
- `card_maker.c`: Utility to convert text source to card deck format.
- `tape_maker.c`: Utility to convert text source to binary tape format.
- `test_source.txt`: Example program (Bootstrap format).
- `lib_source.txt`: Example library function.

## Example

The included example demonstrates the Overlay mechanism:
1. The program loads `5` into `R1`.
2. It calls a library function (Index 0) which is dynamically loaded to address `200`.
3. The library function squares `R1`.
4. The result `25` is printed to the paper tape.
