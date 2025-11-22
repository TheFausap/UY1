#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cpu.h"

// Reusing parsing logic from card_maker
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
        printf("Usage: %s <input_source.txt> <output_tape.bin>\n", argv[0]);
        return 1;
    }

    FILE *in = fopen(argv[1], "r");
    FILE *out = fopen(argv[2], "wb");
    
    if (!in || !out) {
        perror("Error opening files");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), in)) {
        char token1[128];
        char token2[128];
        int count = sscanf(line, "%s %s", token1, token2);
        
        word_t val = 0;
        
        if (count > 0) {
            int op = parse_opcode(token1);
            if (op != 0) {
                long operand = 0;
                if (count == 2) {
                    operand = strtol(token2, NULL, 0);
                }
                val = ((word_t)op << 40) | (operand & 0xFFFFFFFFFF);
            } else {
                val = strtoll(token1, NULL, 0);
            }
        }
        
        val = val & WORD_MASK;
        fwrite(&val, sizeof(word_t), 1, out);
    }

    fclose(in);
    fclose(out);
    return 0;
}
