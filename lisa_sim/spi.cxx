#include "spi.h"

#include <cstdio>

// ---------------------------------------------------------------- SpiFlash

SpiFlash::SpiFlash(const std::string& path, uint32_t size)
    : path_(path), mem_(size, 0xFF)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (f) {
        size_t n = fread(mem_.data(), 1, mem_.size(), f);
        (void) n;               // a shorter file: the rest erased
        fclose(f);
    }
    ok_ = true;
}

SpiFlash::~SpiFlash()
{
    flush();
}

void SpiFlash::flush()
{
    if (!dirty_ || path_.empty())
        return;
    FILE* f = fopen(path_.c_str(), "wb");
    if (!f) {
        fprintf(stderr, "spi flash: cannot write %s\n", path_.c_str());
        return;
    }
    fwrite(mem_.data(), 1, mem_.size(), f);
    fclose(f);
    dirty_ = false;
}

void SpiFlash::select()
{
    n_ = 0;
    cmd_ = 0;
    addr_ = 0;
}

void SpiFlash::erase(uint32_t base, uint32_t len)
{
    uint32_t mask = (uint32_t) mem_.size() - 1;
    base &= mask & ~(len - 1);
    for (uint32_t i = 0; i < len && base + i < mem_.size(); i++)
        mem_[base + i] = 0xFF;
    dirty_ = true;
}

void SpiFlash::deselect()
{
    // an erase runs when the select goes high after its whole address
    if (n_ == 4 && wel_) {
        switch (cmd_) {
        case 0x20: erase(addr_, 0x1000);  break;
        case 0x52: erase(addr_, 0x8000);  break;
        case 0xD8: erase(addr_, 0x10000); break;
        default: break;
        }
    }
    if (n_ >= 1 && (cmd_ == 0xC7 || cmd_ == 0x60) && wel_)
        erase(0, (uint32_t) mem_.size());
    if (n_ >= 1 && cmd_ != 0x06 && cmd_ != 0x05 && cmd_ != 0x35 && cmd_ != 0x9F &&
        cmd_ != 0x03 && cmd_ != 0x0B && cmd_ != 0x90 && cmd_ != 0xAB)
        wel_ = false;           // a program or erase ends the write enable
    n_ = 0;
}

uint8_t SpiFlash::xfer(uint8_t mosi)
{
    uint32_t mask = (uint32_t) mem_.size() - 1;
    uint32_t n = n_++;
    if (n == 0) {
        cmd_ = mosi;
        if (cmd_ == 0x06) wel_ = true;
        if (cmd_ == 0x04) wel_ = false;
        return 0xFF;            // MISO floats under the command: the pull-up
    }
    if (n <= 3 && cmd_ != 0x9F && cmd_ != 0x05 && cmd_ != 0x35) {
        addr_ = (addr_ << 8) | mosi;
        return 0xFF;
    }
    switch (cmd_) {
    case 0x9F: {                // JEDEC ID: Winbond, W25Q128JV
        static const uint8_t id[3] = { 0xEF, 0x40, 0x18 };
        return n <= 3 ? id[n - 1] : 0xFF;
    }
    case 0x90: return (n & 1) ? 0x17 : 0xEF;
    case 0xAB: return 0x17;
    case 0x05: return wel_ ? 0x02 : 0x00;
    case 0x35: return 0x02;     // QE set, as the parts ship
    case 0x03: return mem_[addr_++ & mask];
    case 0x0B:
        if (n == 4)
            return 0xFF;        // the dummy byte
        return mem_[addr_++ & mask];
    case 0x02:
        if (wel_) {
            uint32_t a = addr_ & mask;
            uint32_t p = (a & ~0xFFu) | ((a + (n - 4)) & 0xFFu);
            mem_[p] &= mosi;
            dirty_ = true;
        }
        return 0xFF;
    default:
        return 0xFF;
    }
}

// ----------------------------------------------------------------- LisaSpi

LisaSpi::LisaSpi()
{
    reset();
}

void LisaSpi::reset()
{
    set_ctrl(0);
    div_ = 3;
    data_ = 0;
    left_ = 0;
    done_ = false;
    owned_ = false;
}

void LisaSpi::set_ctrl(uint8_t val)
{
    for (int i = 0; i < 2; i++) {
        bool was = ctrl_ & (1 << i), now = val & (1 << i);
        if (dev_[i] && now && !was)
            dev_[i]->select();
        if (dev_[i] && was && !now)
            dev_[i]->deselect();
    }
    ctrl_ = val;
    owned_ = (val & 0x10) || (owned_ && left_);   // granted at once: no TTLC here
}

void LisaSpi::finish()
{
    uint8_t rx = 0xFF;          // nothing selected: MISO's pull-up
    for (int i = 0; i < 2; i++)
        if ((ctrl_ & (1 << i)) && dev_[i])
            rx &= dev_[i]->xfer(data_);
    data_ = rx;
    done_ = true;
    if (!(ctrl_ & 0x10))
        owned_ = false;
}

void LisaSpi::tick()
{
    if (left_ && --left_ == 0)
        finish();
}

uint8_t LisaSpi::read(uint8_t addr)
{
    switch (addr) {
    case 0x28:
        done_ = false;
        return data_;
    case 0x29: return ctrl_;
    case 0x2A: return div_;
    case 0x2B: return (owned_ ? 0x04 : 0) | (done_ ? 0x02 : 0) | (left_ ? 0x01 : 0);
    default:   return 0;
    }
}

void LisaSpi::write(uint8_t addr, uint8_t val)
{
    switch (addr) {
    case 0x28:
        done_ = false;
        if (!left_) {           // a write while BUSY is ignored, as the RTL
            data_ = val;
            left_ = 16u * (div_ + 1u) + 2u;
        }
        break;
    case 0x29: set_ctrl(val); break;
    case 0x2A: div_ = val; break;
    default: break;
    }
}
