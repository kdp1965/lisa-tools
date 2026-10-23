#include "spi.h"

#include <cstdio>
#include <cstdlib>

// LISA_SPI_TRACE=1: each command to stderr when its select ends
static bool spi_trace()
{
    static int on = -1;
    if (on < 0)
        on = getenv("LISA_SPI_TRACE") != nullptr;
    return on;
}

// ---------------------------------------------------------------- SpiFlash

SpiFlash::SpiFlash(const std::string& path, uint32_t size, bool sst)
    : path_(path), mem_(size, 0xFF), sst_(sst), bp_(sst ? 0x3C : 0)
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
    if (spi_trace() && n_ > 0)
        fprintf(stderr, "spi flash: cmd %02X addr %06X bytes %u wel %d bp %02X\n",
                cmd_, addr_ & 0xFFFFFF, n_, wel_, bp_);
    // an erase runs when the select goes high after its whole address
    bool prot = (bp_ & 0x3C) != 0;
    if (n_ == 4 && wel_ && !prot) {
        switch (cmd_) {
        case 0x20: erase(addr_, 0x1000);  break;
        case 0x52: erase(addr_, 0x8000);  break;
        case 0xD8: erase(addr_, 0x10000); break;
        default: break;
        }
    }
    if (n_ >= 1 && (cmd_ == 0xC7 || cmd_ == 0x60) && wel_ && !prot)
        erase(0, (uint32_t) mem_.size());
    if (n_ >= 1 && cmd_ != 0x06 && cmd_ != 0x05 && cmd_ != 0x35 && cmd_ != 0x9F &&
        cmd_ != 0x03 && cmd_ != 0x0B && cmd_ != 0x90 && cmd_ != 0xAB && cmd_ != 0x50)
        wel_ = false;           // a program, erase or status write ends the write enable
    if (n_ >= 1 && cmd_ != 0x50)
        ewsr_ = false;          // EWSR enables only the next command
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
        if (cmd_ == 0x50) ewsr_ = true;
        return 0xFF;            // MISO floats under the command: the pull-up
    }
    if (cmd_ == 0x01) {         // WRSR: the protection bits, after WREN or (SST) EWSR
        if (n == 1 && (wel_ || ewsr_))
            bp_ = mosi & 0xBC;
        return 0xFF;
    }
    if (n <= 3 && cmd_ != 0x9F && cmd_ != 0x05 && cmd_ != 0x35) {
        addr_ = (addr_ << 8) | mosi;
        return 0xFF;
    }
    switch (cmd_) {
    case 0x9F: {                // JEDEC ID: Winbond W25Q128JV, or SST25VF064C
        static const uint8_t id[3] = { 0xEF, 0x40, 0x18 };
        static const uint8_t sid[3] = { 0xBF, 0x25, 0x4B };
        return n <= 3 ? (sst_ ? sid : id)[n - 1] : 0xFF;
    }
    case 0x90: return sst_ ? ((n & 1) ? 0x4B : 0xBF) : ((n & 1) ? 0x17 : 0xEF);
    case 0xAB: return sst_ ? ((n & 1) ? 0x4B : 0xBF) : 0x17;   // SST: as 90 (the board reads BF at address 0)
    case 0x05: return bp_ | (wel_ ? 0x02 : 0x00);
    case 0x35: return 0x02;     // QE set, as the parts ship
    case 0x03: return mem_[addr_++ & mask];
    case 0x0B:
        if (n == 4)
            return 0xFF;        // the dummy byte
        return mem_[addr_++ & mask];
    case 0x02:
        if (wel_ && !(bp_ & 0x3C)) {
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
