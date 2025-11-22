#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cpu.h"

// Simple parser to convert text input to "cards"
// Format:
// TYPE VALUE
// TYPE: DATA or INST
// VALUE: Integer or Opcode Name + Operand

// For simplicity, we'll just accept raw integer values for now, 
// or a simple format.
// Actually, the user said:
// 1) Data
// 2) store op
// 3) Data
// 4) store op
// ...
// So the deck is just a sequence of 48-bit words.
// This tool will take a text file with one value per line (hex or decimal)
// and write it to the deck file (text format, one per line, to be read by cpu).

// Actually, the simulator reads "cards". 
// "odd cards are loaded directly in r1, even cards are executed"
// So the deck file should just be a list of numbers/instructions.

// Let's make a helper that can translate mnemonics to numbers.

int parse_opcode(char *str) {
    if (strcmp(str, "LOAD_R1") == 0) return OP_LOAD_R1;
    if (strcmp(str, "LOAD_R2") == 0) return OP_LOAD_R2;
    if (strcmp(str, "LOAD_R3") == 0) return OP_LOAD_R3;
    if (strcmp(str, "STORE_R1") == 0) return OP_STORE_R1;
    if (strcmp(str, "STORE_R3") == 0) return OP_STORE_R3;
    if (strcmp(str, "CLEAR_R1") == 0) return OP_CLEAR_R1;
    if (strcmp(str, "CLEAR_R2") == 0) return OP_CLEAR_R2;
    if (strcmp(str, "CLEAR_R3") == 0) return OP_CLEAR_R3;
    if (strcmp(str, "ADD") == 0) return OP_ADD;
    if (strcmp(str, "NEG") == 0) return OP_NEG;
    if (strcmp(str, "MULT") == 0) return OP_MULT;
    if (strcmp(str, "DIV") == 0) return OP_DIV;
    if (strcmp(str, "ROUND") == 0) return OP_ROUND;
    if (strcmp(str, "AND") == 0) return OP_AND;
    if (strcmp(str, "OR") == 0) return OP_OR;
    if (strcmp(str, "XOR") == 0) return OP_XOR;
    if (strcmp(str, "SHIFT") == 0) return OP_SHIFT;
    if (strcmp(str, "CALL") == 0) return OP_CALL;
    if (strcmp(str, "RET") == 0) return OP_RET;
    if (strcmp(str, "WRITE_PT") == 0) return OP_WRITE_PT;
    if (strcmp(str, "READ_CR") == 0) return OP_READ_CR;
    if (strcmp(str, "SKIP") == 0) return OP_SKIP;
    if (strcmp(str, "SKIP_Z") == 0) return OP_SKIP_Z;
    if (strcmp(str, "SKIP_NZ") == 0) return OP_SKIP_NZ;
    if (strcmp(str, "TXR") == 0) return OP_TXR;
    if (strcmp(str, "HALT") == 0) return OP_HALT;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_source.txt> <output_deck.txt>\n", argv[0]);
        return 1;
    }

    FILE *in = fopen(argv[1], "r");
    FILE *out = fopen(argv[2], "w");
    
    if (!in || !out) {
        perror("Error opening files");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), in)) {
        // Try to parse as "OPCODE OPERAND" or just "VALUE"
        char token1[128];
        char token2[128];
        int count = sscanf(line, "%s %s", token1, token2);
        
        word_t val = 0;
        
        if (count > 0) {
            int op = parse_opcode(token1);
            if (op != 0) {
                // It's an instruction
                long operand = 0;
                if (count == 2) {
                    operand = strtol(token2, NULL, 0);
                }
                // Encode: Opcode in top 8 bits? 
                // Let's define format: [8 bits Opcode] [40 bits Operand]
                val = ((word_t)op << 40) | (operand & 0xFFFFFFFFFF);
            } else {
                // It's data
                val = strtoll(token1, NULL, 0);
            }
        }
        
        // Mask to 48 bits
        val = val & WORD_MASK;
        fprintf(out, "%ld\n", val);
    }

    fclose(in);
    fclose(out);
    return 0;
}
