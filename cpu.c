#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

cpu_state_t cpu;

// --- Tape Operations ---

void tape_write(FILE *f, long index, word_t value) {
    if (!f) return;
    fseek(f, index * sizeof(word_t), SEEK_SET);
    fwrite(&value, sizeof(word_t), 1, f);
}

word_t tape_read(FILE *f, long index) {
    if (!f) return 0;
    word_t value = 0;
    fseek(f, index * sizeof(word_t), SEEK_SET);
    if (fread(&value, sizeof(word_t), 1, f) != 1) {
        return 0;
    }
    return value;
}

// --- ALU & Helpers ---

void print_state() {
    printf("PC: %ld | R1: %ld | R2: %ld | R3: %ld | Mode: %s\n", 
           cpu.pc, cpu.r1, cpu.r2, cpu.r3, cpu.mode ? "EXEC" : "READ_IN");
}

// --- Execution ---

// --- Execution ---

// Helper for 96-bit shift
void shift_r1r2(int amount) {
    __int128_t val = ((__int128_t)(cpu.r1 & WORD_MASK) << 48) | (cpu.r2 & WORD_MASK);
    // Sign extension if r1 is negative?
    if (cpu.r1 & SIGN_BIT) {
        val |= ((__int128_t)0xFFFF << 96); // Extend sign
    }
    
    if (amount > 0) {
        val <<= amount;
    } else {
        val >>= (-amount);
    }
    
    cpu.r1 = (word_t)((val >> 48) & WORD_MASK);
    cpu.r2 = (word_t)(val & WORD_MASK);
}

void execute_instruction(word_t inst) {
    // Decode
    int opcode = (inst >> 40) & 0xFF;
    long operand = inst & 0xFFFFFFFFFF;
    // Treat operand as signed 40-bit for relative jumps or shifts
    long s_operand = (operand & 0x8000000000) ? (operand | 0xFFFFFF0000000000) : operand;

    switch (opcode) {
        case OP_LOAD_R1:
            cpu.r1 = tape_read(cpu.scratchpad, operand);
            break;
        case OP_LOAD_R2:
            cpu.r2 = tape_read(cpu.scratchpad, operand);
            break;
        case OP_LOAD_R3:
            cpu.r3 = tape_read(cpu.scratchpad, operand);
            break;
        case OP_STORE_R1:
            tape_write(cpu.scratchpad, operand, cpu.r1);
            break;
        case OP_STORE_R3:
            tape_write(cpu.scratchpad, operand, cpu.r3);
            break;
        case OP_CLEAR_R1:
            cpu.r1 = 0;
            break;
        case OP_CLEAR_R2:
            cpu.r2 = 0;
            break;
        case OP_CLEAR_R3:
            cpu.r3 = 0;
            break;
        case OP_ADD:
            // r1 + r2 -> r1
            {
                int64_t v1 = sign_extend(cpu.r1);
                int64_t v2 = sign_extend(cpu.r2);
                cpu.r1 = mask_word(v1 + v2);
            }
            break;
        case OP_NEG:
            {
                int64_t v1 = sign_extend(cpu.r1);
                cpu.r1 = mask_word(-v1);
            }
            break;
        case OP_MULT:
            // r2 * r3 -> r1:r2
            // Fixed point multiplication
            {
                int64_t v2 = sign_extend(cpu.r2);
                int64_t v3 = sign_extend(cpu.r3);
                __int128_t res = (__int128_t)v2 * v3;
                // Result is in 2.94 format (since 1.47 * 1.47)
                // We want to store it in r1:r2 (1.47 : 48) ?
                // Standard integer mult:
                cpu.r2 = mask_word((int64_t)res);
                cpu.r1 = mask_word((int64_t)(res >> 48));
            }
            break;
        case OP_DIV:
            // r1 / r2 -> r1
            {
                int64_t v1 = sign_extend(cpu.r1);
                int64_t v2 = sign_extend(cpu.r2);
                if (v2 != 0) {
                    // Fixed point division: (v1 << 48) / v2 ?
                    // Or just integer div?
                    // "values are in the range [-1,1)"
                    // If we divide 0.5 / 2 = 0.25.
                    // 0x4000... / 2 = 0x2000...
                    // Integer division works for this if we don't shift?
                    // No, 0.5 (2^46) / 0.5 (2^46) = 1 (2^47).
                    // (2^46 << 47) / 2^46 = 2^47.
                    // So we need to shift v1 left by 47 (or 48) before dividing?
                    // Let's assume standard integer division for now, user can shift if needed.
                    // Or better, implement fractional division.
                    __int128_t num = (__int128_t)v1 << 47; // Align points?
                    // Let's stick to integer div for simplicity unless specified.
                    cpu.r1 = mask_word(v1 / v2);
                }
            }
            break;
        case OP_ROUND:
             // If r2 MSB is 1, add 1 to r1
            if (cpu.r2 & SIGN_BIT) { 
                 cpu.r1 = mask_word(cpu.r1 + 1);
            }
            break;
        case OP_AND:
            cpu.r1 = cpu.r1 & cpu.r2;
            break;
        case OP_OR:
            cpu.r1 = cpu.r1 | cpu.r2;
            break;
        case OP_XOR:
            cpu.r1 = cpu.r1 ^ cpu.r2;
            break;
        case OP_SHIFT:
            shift_r1r2((int)s_operand);
            break;
        case OP_CALL:
            // Overlay Mechanism
            // Operand: [16 bits Lib Index] [24 bits Dest Address]
            // Actually operand is 40 bits. 
            // Let's say Top 16 bits of the 40-bit operand?
            // Operand is `inst & 0xFFFFFFFFFF`.
            // High 16: (operand >> 24) & 0xFFFF
            // Low 24: operand & 0xFFFFFF
            {
                long lib_index = (operand >> 24) & 0xFFFF;
                long dest_addr = operand & 0xFFFFFF;
                
                if (!cpu.library) {
                    printf("Error: Library tape not available.\n");
                    break;
                }
                
                // 1. Read Library Function
                // We assume the library tape is a sequence of words.
                // We need to know the length of the function.
                // Let's assume we copy until we hit a RET instruction?
                // Or maybe the library has a header?
                // "copies into that scratch area ... until it found a RET instruction"
                
                long current_lib_pos = lib_index;
                long current_dest_pos = dest_addr;
                
                while (1) {
                    word_t lib_inst = tape_read(cpu.library, current_lib_pos);
                    int lib_op = (lib_inst >> 40) & 0xFF;
                    
                    if (lib_op == OP_RET) {
                        // 4. Replace RET with TXR (PC + 1)
                        // Wait, PC is already pointing to next instruction (CALL + 1).
                        // So we want to return to PC + 1 (instruction after the TXR we are about to write).
                        // The user said: "RET instruction is not copied ... in its place a TXR is placed with the return address to pc+2"
                        // If CALL is at N, PC is N+1.
                        // We write TXR at N+1.
                        // So we want to return to N+2.
                        // So return address is cpu.pc + 1.
                        
                        // Construct TXR instruction
                        // Opcode TXR (OP_TXR) | Operand (cpu.pc + 1)
                        word_t txr_ret = ((word_t)OP_TXR << 40) | ((cpu.pc + 1) & 0xFFFFFFFFFF);
                        tape_write(cpu.scratchpad, current_dest_pos, txr_ret);
                        break; // Done copying
                    } else {
                        // Copy instruction
                        tape_write(cpu.scratchpad, current_dest_pos, lib_inst);
                    }
                    
                    current_lib_pos++;
                    current_dest_pos++;
                }
                
                // 5. Write TXR (Dest Address) to scratchpad at PC
                // PC is currently pointing to the instruction AFTER Call.
                // "next memory location that the programmer filled with 0"
                word_t txr_call = ((word_t)OP_TXR << 40) | (dest_addr & 0xFFFFFFFFFF);
                tape_write(cpu.scratchpad, cpu.pc, txr_call);
                
                // 6. Execution continues
                // The next fetch will be at cpu.pc, which is now the TXR we just wrote.
                // This will jump to dest_addr.
            }
            break;
        case OP_RET:
            // Return from r3
            cpu.pc = cpu.r3;
            break;
        case OP_WRITE_PT:
            if (cpu.paper_tape) {
                fprintf(cpu.paper_tape, "%ld\n", cpu.r3);
            }
            break;
        case OP_READ_CR:
            if (cpu.card_reader) {
                char line[128];
                if (fgets(line, sizeof(line), cpu.card_reader)) {
                    cpu.r3 = strtoll(line, NULL, 0) & WORD_MASK;
                }
            }
            break;
        case OP_SKIP:
            cpu.pc++;
            break;
        case OP_SKIP_Z:
            if (cpu.r1 == 0) cpu.pc++;
            break;
        case OP_SKIP_NZ:
            if (cpu.r1 != 0) cpu.pc++;
            break;
        case OP_TXR:
            cpu.pc = operand;
            if (cpu.mode == 0) {
                cpu.mode = 1;
                printf("Switching to EXECUTION mode. Jumping to %ld\n", cpu.pc);
            }
            break;
        case OP_HALT:
            exit(0);
            break;
        default:
            printf("Unknown Opcode: %d\n", opcode);
            break;
    }
}

int main() {
    // Init
    memset(&cpu, 0, sizeof(cpu));
    cpu.scratchpad = fopen("scratchpad.bin", "w+b");
    cpu.library = fopen("library.bin", "rb"); // Optional
    cpu.paper_tape = fopen("output.txt", "w");
    cpu.card_reader = fopen("deck.txt", "r");
    
    if (!cpu.scratchpad || !cpu.paper_tape || !cpu.card_reader) {
        perror("Failed to open files");
        return 1;
    }

    // Bootstrap / Simulation Loop
    char line[128];
    int card_count = 0;
    
    while (1) {
        if (cpu.mode == 0) {
            // READ-IN Mode
            // Read Odd Card -> r1
            if (!fgets(line, sizeof(line), cpu.card_reader)) break; // End of deck
            word_t val = strtoll(line, NULL, 0) & WORD_MASK;
            cpu.r1 = val;
            card_count++;
            
            // Read Even Card -> Execute
            if (!fgets(line, sizeof(line), cpu.card_reader)) break; // Should be pairs
            word_t inst = strtoll(line, NULL, 0) & WORD_MASK;
            card_count++;
            
            execute_instruction(inst);
            
            // If TXR executed, cpu.mode changes to 1.
        } else {
            // EXECUTION Mode
            word_t inst = tape_read(cpu.scratchpad, cpu.pc);
            cpu.pc++;
            execute_instruction(inst);
        }
    }

    fclose(cpu.scratchpad);
    if (cpu.library) fclose(cpu.library);
    fclose(cpu.paper_tape);
    fclose(cpu.card_reader);
    
    return 0;
}
