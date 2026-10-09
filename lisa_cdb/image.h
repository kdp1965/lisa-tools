// image.h - a LISA program image from a file, as 16-bit instruction words:
// Intel HEX (sdcc -mlisa: byte addresses, little-endian words), the
// cocotb firmware.hex form (@addr then hex bytes) or one 4-digit hex
// opcode per line (the bring-up demos).  Shared by lisa_ide and lisa_sim.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace lisa {

// words is sized to the highest address written; false with err on failure
bool load_hex_image(const std::string& path, std::vector<uint16_t>& words, std::string* err = nullptr);

}  // namespace lisa
