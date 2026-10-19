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
    // The physical spaces of the second chip (--rev2, RESPIN 22): the upper
    // The upper 16K of the code space is a window on one of 64 code banks
    // (20-bit physical words); the data space is the common 8K, the data
    // window 0x2000-0x3FFF on one of 32 granules of 8K and the stack window
    // 0x4000 up on one of 16 banks of 16K (18-bit physical bytes;
    // LisaCore::code_pa / data_pa); TT07's 32K is physical 0-32K
    static constexpr int INST_SIZE = 1 << 20;  // instruction words: 64 code banks of 16K
    static constexpr int DATA_SIZE = 1 << 18;  // data bytes
    static constexpr int PC_BITS   = 15;
    static constexpr int D_BITS    = 15;

    LisaMemory();
    void reset();

    // Instruction memory
    uint16_t inst_read(uint32_t addr) const;     // physical addresses
    void inst_write(uint32_t addr, uint16_t data);

    // Data memory
    uint8_t data_read(uint32_t addr) const;
    void data_write(uint32_t addr, uint8_t data);

    // Bulk access for debugging
    const uint16_t* inst_mem() const { return inst_.data(); }
    const uint8_t*  data_mem() const { return data_.data(); }
    uint16_t* inst_mem_mut() { return inst_.data(); }
    uint8_t*  data_mem_mut() { return data_.data(); }

    // Load firmware from hex file
    // Supports format: lines of space-separated hex bytes, @ADDR for base address
    // (matches firmware.hex format used by testbench)
    bool load_hex(const std::string& filename);

    // Load raw binary
    bool load_bin(const std::string& filename);

    // Get firmware size (number of 16-bit words loaded)
    size_t firmware_words() const { return fw_words_; }

    // Limit the data RAM to a power-of-two size (the TT07 chip has 128
    // bytes without the cache): addresses wrap, as they do on the chip
    void set_data_size(size_t bytes) { data_mask_ = (uint32_t)(bytes - 1); }
    size_t data_size() const { return (size_t)data_mask_ + 1; }

private:
    std::vector<uint16_t> inst_;
    std::vector<uint8_t>  data_;
    size_t   fw_words_;
    uint32_t data_mask_ = DATA_SIZE - 1;
};

#endif
