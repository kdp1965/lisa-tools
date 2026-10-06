#ifndef LISA_DISASM_H
#define LISA_DISASM_H

#include <cstdint>
#include <string>

// Disassemble a single 16-bit LISA instruction to human-readable mnemonic
std::string lisa_disasm(uint16_t inst, uint16_t pc = 0, uint16_t next_word = 0);

#endif
