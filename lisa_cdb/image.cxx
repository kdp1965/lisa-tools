// image.cxx - see image.h
#include "image.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

namespace lisa {

bool load_hex_image(const std::string& path, std::vector<uint16_t>& words, std::string* err) {
    FILE* fp = fopen(path.c_str(), "r");
    if (!fp) {
        if (err) *err = "cannot open " + path;
        return false;
    }
    std::vector<uint8_t> bytes;
    char line[1024];
    bool intel = false, opcodes = true;
    if (fgets(line, sizeof line, fp)) {
        intel = line[0] == ':';
        rewind(fp);
    }
    if (intel) {                                        // :LLAAAARR[DD...]CC
        while (fgets(line, sizeof line, fp)) {
            if (line[0] != ':') continue;
            unsigned len, addr, type;
            if (sscanf(line + 1, "%02x%04x%02x", &len, &addr, &type) != 3) continue;
            if (type == 1) break;
            if (type != 0) continue;
            if (addr + len > bytes.size()) bytes.resize(addr + len, 0);
            for (unsigned i = 0; i < len; i++) {
                unsigned v;
                if (sscanf(line + 9 + 2 * i, "%02x", &v) == 1) bytes[addr + i] = (uint8_t)v;
            }
        }
    } else {
        // @addr + hex bytes, or bare 4-digit words: told apart by the tokens
        size_t off = 0;
        std::vector<std::string> toks;
        while (fgets(line, sizeof line, fp)) {
            char* p = line;
            while (*p) {
                while (*p && isspace((unsigned char)*p)) p++;
                if (!*p) break;
                char* s = p;
                while (*p && !isspace((unsigned char)*p)) p++;
                toks.emplace_back(s, p - s);
            }
        }
        for (const std::string& t : toks)
            if (t[0] != '@' && t.size() != 4) opcodes = false;
        if (toks.empty()) opcodes = false;
        if (opcodes) {
            for (const std::string& t : toks) {
                if (t[0] == '@') { off = strtoul(t.c_str() + 1, nullptr, 16); continue; }
                if (off >= words.size()) words.resize(off + 1, 0);
                words[off++] = (uint16_t)strtoul(t.c_str(), nullptr, 16);
            }
        } else {
            for (const std::string& t : toks) {
                if (t[0] == '@') { off = strtoul(t.c_str() + 1, nullptr, 16); continue; }
                if (off >= bytes.size()) bytes.resize(off + 1, 0);
                bytes[off++] = (uint8_t)strtoul(t.c_str(), nullptr, 16);
            }
        }
    }
    fclose(fp);
    if (!bytes.empty()) {
        words.assign((bytes.size() + 1) / 2, 0);
        for (size_t i = 0; i < words.size(); i++) {
            uint8_t lo = bytes[2 * i], hi = 2 * i + 1 < bytes.size() ? bytes[2 * i + 1] : 0;
            words[i] = (uint16_t)(lo | (hi << 8));
        }
    }
    if (words.empty()) {
        if (err) *err = path + ": no program in it";
        return false;
    }
    if (words.size() > 32768) words.resize(32768);
    return true;
}

}  // namespace lisa
