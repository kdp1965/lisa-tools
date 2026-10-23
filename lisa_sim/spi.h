#ifndef LISA_SPI_H
#define LISA_SPI_H

// The third chip's SPI master (ttsky26d RESPIN 29, src/lisa_spi.v):
// registers 0x28-0x2B, a byte per 16 x (DIV + 1) clocks, DONE as interrupt
// source 7, and the devices on its two selects.  The TTLC's pins (CLAIM)
// are granted at once: the simulator has no TTLC, so nothing ever waits.

#include <cstdint>
#include <string>
#include <vector>

// A device on a select, at byte level (the clock modes are the RTL's concern)
class SpiDevice {
public:
    virtual ~SpiDevice() {}
    virtual void select() = 0;
    virtual void deselect() = 0;
    virtual uint8_t xfer(uint8_t mosi) = 0;
};

// A SPI NOR flash like the W25Q128JV: 9F / 90 / AB identify it, 05 / 35
// status, 06 / 04 the write enable, 03 / 0B read, 02 page program (inside
// the 256-byte page, bits only cleared), 20 / 52 / D8 erase 4K / 32K / 64K,
// C7 / 60 the chip.  Erase and program take no time (BUSY never shows).
// Backed by a file, written back when the simulator exits.  With sst set it
// is an SST25VF064C instead (BF 25 4B): the block protection bits of the
// status register are set at power-up and keep the array from programs and
// erases until EWSR (50) or WREN and WRSR (01) clear them.
class SpiFlash : public SpiDevice {
public:
    SpiFlash(const std::string& path, uint32_t size, bool sst = false);
    ~SpiFlash();
    bool ok() const { return ok_; }
    void select() override;
    void deselect() override;
    uint8_t xfer(uint8_t mosi) override;
    void flush();

private:
    std::string path_;
    std::vector<uint8_t> mem_;
    bool ok_ = false;
    bool dirty_ = false;
    bool wel_ = false;
    bool sst_ = false;
    bool ewsr_ = false;          // SST: the status register write enabled (EWSR)
    uint8_t bp_ = 0;             // the status register's bits 7:2 (block protection)
    uint8_t cmd_ = 0;
    uint32_t n_ = 0;             // bytes since the select
    uint32_t addr_ = 0;
    void erase(uint32_t base, uint32_t len);
};

class LisaSpi {
public:
    LisaSpi();
    void reset();
    uint8_t read(uint8_t addr);
    void write(uint8_t addr, uint8_t val);
    void tick();
    bool irq() const { return done_ && (ctrl_ & 0x80); }
    void attach(int cs, SpiDevice* dev) { dev_[cs & 1] = dev; }

private:
    uint8_t ctrl_ = 0;
    uint8_t div_ = 3;
    uint8_t data_ = 0;
    uint32_t left_ = 0;          // clocks until the byte is done
    bool done_ = false;
    bool owned_ = false;
    SpiDevice* dev_[2] = {nullptr, nullptr};
    void set_ctrl(uint8_t val);
    void finish();
};

#endif
