#include "memory.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

LisaMemory::LisaMemory() : fw_words_(0) {
    reset();
}

void LisaMemory::reset() {
    std::memset(inst_, 0, sizeof(inst_));
    std::memset(data_, 0, sizeof(data_));
    fw_words_ = 0;
}

uint16_t LisaMemory::inst_read(uint16_t addr) const {
    return inst_[addr & (INST_SIZE - 1)];
}

void LisaMemory::inst_write(uint16_t addr, uint16_t data) {
    inst_[addr & (INST_SIZE - 1)] = data;
}

uint8_t LisaMemory::data_read(uint16_t addr) const {
    return data_[addr & (DATA_SIZE - 1)];
}

void LisaMemory::data_write(uint16_t addr, uint8_t data) {
    data_[addr & (DATA_SIZE - 1)] = data;
}

bool LisaMemory::load_hex(const std::string& filename) {
    FILE* fp = fopen(filename.c_str(), "r");
    if (!fp) return false;

    // Collect all raw bytes first
    std::vector<uint8_t> bytes;
    size_t base_addr = 0;
    char line[1024];
    bool is_intel_hex = false;

    // Peek at first line to detect format
    if (fgets(line, sizeof(line), fp)) {
        if (line[0] == ':') {
            is_intel_hex = true;
        }
        rewind(fp);
    }

    if (is_intel_hex) {
        // Intel HEX format: :LLAAAARR[DD...]CC
        while (fgets(line, sizeof(line), fp)) {
            if (line[0] != ':') continue;
            unsigned len, addr, type;
            if (sscanf(line + 1, "%02x%04x%02x", &len, &addr, &type) != 3)
                continue;
            if (type == 0x01) break;  // EOF
            if (type != 0x00) continue;  // Only data records

            // Ensure bytes vector is large enough
            if (addr + len > bytes.size())
                bytes.resize(addr + len, 0);

            for (unsigned i = 0; i < len; i++) {
                unsigned val;
                if (sscanf(line + 9 + i * 2, "%02x", &val) == 1)
                    bytes[addr + i] = (uint8_t)val;
            }
        }
    } else {
        // Simple hex format: @ADDR then lines of space-separated hex bytes
        size_t byte_offset = 0;
        while (fgets(line, sizeof(line), fp)) {
            // Strip trailing whitespace
            char* end = line + strlen(line) - 1;
            while (end >= line && (*end == '\n' || *end == '\r' || *end == ' '))
                *end-- = '\0';
            if (line[0] == '\0') continue;

            if (line[0] == '@') {
                base_addr = strtoul(line + 1, nullptr, 16);
                byte_offset = base_addr;
                continue;
            }

            // Parse space-separated hex bytes
            char* tok = strtok(line, " \t");
            while (tok) {
                unsigned val;
                if (sscanf(tok, "%x", &val) == 1) {
                    if (byte_offset >= bytes.size())
                        bytes.resize(byte_offset + 1, 0);
                    bytes[byte_offset++] = (uint8_t)val;
                }
                tok = strtok(nullptr, " \t");
            }
        }
    }

    fclose(fp);

    if (bytes.empty()) return false;

    // Convert byte pairs to 16-bit words (little-endian: byte0=low, byte1=high)
    fw_words_ = (bytes.size() + 1) / 2;
    if (fw_words_ > INST_SIZE) fw_words_ = INST_SIZE;

    for (size_t i = 0; i < fw_words_; i++) {
        uint8_t lo = (i * 2 < bytes.size()) ? bytes[i * 2] : 0;
        uint8_t hi = (i * 2 + 1 < bytes.size()) ? bytes[i * 2 + 1] : 0;
        inst_[i] = ((uint16_t)hi << 8) | lo;
    }

    return true;
}

bool LisaMemory::load_bin(const std::string& filename) {
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) return false;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size <= 0) { fclose(fp); return false; }

    std::vector<uint8_t> buf(size);
    if (fread(buf.data(), 1, size, fp) != (size_t)size) {
        fclose(fp);
        return false;
    }
    fclose(fp);

    fw_words_ = (size + 1) / 2;
    if (fw_words_ > INST_SIZE) fw_words_ = INST_SIZE;

    for (size_t i = 0; i < fw_words_; i++) {
        uint8_t lo = buf[i * 2];
        uint8_t hi = (i * 2 + 1 < (size_t)size) ? buf[i * 2 + 1] : 0;
        inst_[i] = ((uint16_t)hi << 8) | lo;
    }

    return true;
}
