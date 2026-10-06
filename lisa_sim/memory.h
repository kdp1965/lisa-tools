#ifndef LISA_MEMORY_H
#define LISA_MEMORY_H

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

// LISA memory architecture:
// - 32K x 16-bit instruction memory (15-bit PC addresses words)
// - 32K x 8-bit data memory (15-bit address)
// - Peripheral space: when d_periph flag is set, address goes to peripherals not RAM

class LisaMemory {
public:
    static constexpr int INST_SIZE = 32768;   // 2^15 instruction words
    static constexpr int DATA_SIZE = 32768;   // 2^15 data bytes
    static constexpr int PC_BITS   = 15;
    static constexpr int D_BITS    = 15;

    LisaMemory();
    void reset();

    // Instruction memory
    uint16_t inst_read(uint16_t addr) const;
    void inst_write(uint16_t addr, uint16_t data);

    // Data memory
    uint8_t data_read(uint16_t addr) const;
    void data_write(uint16_t addr, uint8_t data);

    // Bulk access for debugging
    const uint16_t* inst_mem() const { return inst_; }
    const uint8_t*  data_mem() const { return data_; }
    uint16_t* inst_mem_mut() { return inst_; }
    uint8_t*  data_mem_mut() { return data_; }

    // Load firmware from hex file
    // Supports format: lines of space-separated hex bytes, @ADDR for base address
    // (matches firmware.hex format used by testbench)
    bool load_hex(const std::string& filename);

    // Load raw binary
    bool load_bin(const std::string& filename);

    // Get firmware size (number of 16-bit words loaded)
    size_t firmware_words() const { return fw_words_; }

private:
    uint16_t inst_[INST_SIZE];
    uint8_t  data_[DATA_SIZE];
    size_t   fw_words_;
};

#endif
