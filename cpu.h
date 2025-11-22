#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

// 48-bit word, stored in int64_t
// We will use the lower 48 bits.
// Fixed point: [-1, 1)
// Bit 47 is sign bit.
typedef int64_t word_t;

// Mask to keep only 48 bits
#define WORD_MASK 0xFFFFFFFFFFFF
// Sign bit mask (bit 47)
#define SIGN_BIT (1ULL << 47)

// Opcodes
typedef enum {
    OP_LOAD_R1 = 1,
    OP_LOAD_R2,
    OP_LOAD_R3,
    OP_STORE_R1,
    OP_STORE_R3,
    OP_CLEAR_R1,
    OP_CLEAR_R2,
    OP_CLEAR_R3,
    OP_ADD,      // r1 + r2 -> r1
    OP_NEG,      // -r1 -> r1
    OP_MULT,     // r2 * r3 -> r1:r2
    OP_DIV,      // r1 / r2 -> r1
    OP_ROUND,    // Round r1:r2
    OP_AND,      // r1 & r2 -> r1
    OP_OR,       // r1 | r2 -> r1
    OP_XOR,      // r1 ^ r2 -> r1
    OP_SHIFT,    // Shift r1:r2. Operand: >0 left, <0 right? Or specific encoding.
    OP_CALL,     // Call library function
    OP_RET,      // Return from library function
    OP_WRITE_PT, // Write r3 to paper tape
    OP_READ_CR,  // Read from card reader to r3
    OP_SKIP,     // Unconditional skip (next instruction)
    OP_SKIP_Z,   // Skip if r1 == 0
    OP_SKIP_NZ,  // Skip if r1 != 0
    OP_TXR,      // Transfer and Execute (Jump)
    OP_HALT = 99 // Stop simulation (not strictly requested but useful)
} Opcode;

// CPU State
typedef struct {
    word_t r1;
    word_t r2;
    word_t r3;
    
    // Program Counter (Tape Head Index)
    // Points to the next instruction in scratchpad
    long pc; 
    
    // Mode: 0 = READ_IN, 1 = EXECUTION
    int mode;

    // Files
    FILE *scratchpad;  // Read/Write
    FILE *library;     // Read Only
    FILE *paper_tape;  // Write Only
    FILE *card_reader; // Read Only
    
} cpu_state_t;

// Helper to sign extend 48-bit to 64-bit
static inline int64_t sign_extend(word_t w) {
    if (w & SIGN_BIT) {
        return w | ~WORD_MASK;
    }
    return w & WORD_MASK;
}

// Helper to mask to 48-bit
static inline word_t mask_word(int64_t w) {
    return w & WORD_MASK;
}

#endif // CPU_H
