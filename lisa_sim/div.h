#ifndef LISA_DIV_H
#define LISA_DIV_H

#include <cstdint>

// 16-bit hardware divider matching lisa_div.v
// Supports signed and unsigned division and remainder
struct LisaDiv {
    // Start a division operation
    // rs1 = dividend (16-bit), rs2 = divisor (16-bit)
    // op: 0=unsigned div, 1=signed div, 2=unsigned rem, 3=signed rem
    void start(uint16_t rs1, uint16_t rs2, int op);

    // Get result (available after start)
    uint16_t result() const { return result_; }
    bool ready() const { return ready_; }

    // For cycle-accurate simulation, call tick() each cycle
    // For functional simulation, result is ready immediately after start()
    void tick();
    void reset();

    LisaDiv();

private:
    uint16_t result_;
    bool     ready_;
};

#endif
