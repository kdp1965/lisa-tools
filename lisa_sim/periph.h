#ifndef LISA_PERIPH_H
#define LISA_PERIPH_H

#include <cstdint>
#include <functional>

#include "spi.h"
#include "timer3.h"
#include "ws2812.h"

class LisaUart;

// Peripheral register addresses (7-bit, matching lisa_periph.v)
namespace PeriphReg {
    constexpr uint8_t PORT_A_IN       = 0x00;
    constexpr uint8_t PORT_B_OUT      = 0x01;
    constexpr uint8_t PORT_C_DATA     = 0x02;
    constexpr uint8_t PORT_C_DIR      = 0x03;
    constexpr uint8_t PORT_D_IN       = 0x04;
    constexpr uint8_t PORT_E_OUT      = 0x05;
    constexpr uint8_t PORT_A_INT_CFG0 = 0x06;
    constexpr uint8_t PORT_A_INT_CFG1 = 0x07;

    // Timer 1
    constexpr uint8_t TIM1_PREDIV_LO  = 0x08;
    constexpr uint8_t TIM1_PREDIV_HI  = 0x09;
    constexpr uint8_t TIM1_DIV_LO     = 0x0A;
    constexpr uint8_t TIM1_DIV_HI     = 0x0B;
    constexpr uint8_t TIM1_CTRL       = 0x0C;
    constexpr uint8_t TIM1_COUNT      = 0x0D;

    constexpr uint8_t INT_ENABLE      = 0x0E;
    constexpr uint8_t INT_STATUS      = 0x0F;

    // UART 1
    constexpr uint8_t UART1_DATA      = 0x10;
    constexpr uint8_t UART1_STATUS    = 0x11;

    // UART 2
    constexpr uint8_t UART2_DATA      = 0x12;
    constexpr uint8_t UART2_STATUS    = 0x13;
    constexpr uint8_t UART_INT_EN     = 0x14;

    // The banks (the second chip, RESPIN 22 / 28): code 6 bits, data 5, stack 4, reset 1
    constexpr uint8_t BANK_CODE       = 0x15;  // fetches above word 0x4000
    constexpr uint8_t BANK_DATA       = 0x16;  // the data window 0x2000-0x3FFF: 32 granules of 8K (RESPIN 28)
    constexpr uint8_t BANK_STACK      = 0x17;  // the stack window 0x4000-0x7FFF: 16 banks of 16K
    constexpr uint8_t BANK_CONST      = 0x1F;  // KB: the bank lda (ix++) reads constants from (ttsky26d RESPIN 26)

    // Timer 2
    constexpr uint8_t TIM2_PREDIV_LO  = 0x18;
    constexpr uint8_t TIM2_PREDIV_HI  = 0x19;
    constexpr uint8_t TIM2_DIV_LO     = 0x1A;
    constexpr uint8_t TIM2_DIV_HI     = 0x1B;
    constexpr uint8_t TIM2_CTRL       = 0x1C;
    constexpr uint8_t TIM2_COUNT      = 0x1D;

    // UART2's baud divisor (the second chip, RESPIN 23: a bit is 32 x (d + 1)
    // clocks; TT07 wrote it on any access to 0x14).  Not modelled: the
    // simulator's UARTs have no baud rate
    constexpr uint8_t UART2_BRG       = 0x1E;

    // I2C
    constexpr uint8_t I2C_PRESCALE_LO = 0x20;
    constexpr uint8_t I2C_PRESCALE_HI = 0x21;
    constexpr uint8_t I2C_CTRL        = 0x22;
    constexpr uint8_t I2C_RX_DATA     = 0x23;
    constexpr uint8_t I2C_STATUS      = 0x24;
    constexpr uint8_t I2C_TX_DATA     = 0x25;
    constexpr uint8_t I2C_CMD         = 0x26;

    // SPI master (the third chip, ttsky26d RESPIN 29: spi.h)
    constexpr uint8_t SPI_DATA        = 0x28;
    constexpr uint8_t SPI_CTRL        = 0x29;
    constexpr uint8_t SPI_DIV         = 0x2A;
    constexpr uint8_t SPI_STATUS      = 0x2B;
}

// Interrupt source bits
namespace IntSrc {
    constexpr uint8_t TIMER1  = 0x01;
    constexpr uint8_t TIMER2  = 0x02;
    constexpr uint8_t UART1   = 0x04;
    constexpr uint8_t UART2   = 0x08;
    constexpr uint8_t GPIO_A  = 0x10;
    constexpr uint8_t TTLC    = 0x20;
    constexpr uint8_t I2C     = 0x40;
    constexpr uint8_t SPI     = 0x80;  // the third chip's SPI master: DONE with IE (a level)
    constexpr uint16_t TIMER3 = 0x100; // the third chip's Timer 3 (RESPIN 32): source 8, vector 9
}

struct TimerState {
    uint16_t prediv;       // Prescaler reload value
    uint16_t divisor;      // Timer reload value
    uint8_t  counter;      // 8-bit down counter
    uint16_t pre_counter;  // Prescaler counter
    bool     enable;       // Timer running
    bool     rollover;     // Overflow flag
};

class LisaPeripherals {
public:
    LisaPeripherals();
    void reset();

    // The bank registers exist on the second chip only (--rev2); the core's
    // code_pa / data_pa read them
    void set_rev2(bool on) { rev2_ = on; }
    void set_rev3(bool on) { rev3_ = on; }   // 7-bit code bank; no data / stack / constant banks (REV3.md)
    uint8_t const_bank() const { return rev2_ ? const_bank_ : 0; }
    uint8_t code_bank() const { return code_bank_; }
    uint8_t data_bank() const { return data_bank_; }
    uint8_t stack_bank() const { return stack_bank_; }

    // Register read/write (called by core when d_periph is set)
    uint8_t read(uint8_t addr);
    void write(uint8_t addr, uint8_t val);

    // Advance one clock cycle (for timers, etc.)
    void tick();

    // Get pending interrupt mask (for core to poll); bit 8 Timer 3 (gated by its own IE)
    uint16_t get_interrupts() const;

    // Connect UARTs
    void set_uart1(LisaUart* u) { uart1_ = u; }
    void set_uart2(LisaUart* u) { uart2_ = u; }

    // GPIO access for external simulation
    void set_port_a(uint8_t val) { port_a_in_ = val; }
    // a square wave on port A bits 4 and 5 (Timer 3's capture inputs),
    // toggling every n clocks (0: off) - the chip testbench's cap_period
    void set_cap_wave(uint32_t n) { cap_wave_ = n; cap_cnt_ = 0; }
    uint8_t get_port_b() const { return port_b_out_; }
    void set_port_d(uint8_t val) { port_d_in_ = val; }
    uint8_t get_port_e() const { return port_e_out_; }
    void set_port_c_input(uint8_t val) { port_c_in_ = val; }

    // The SPI master (--rev3) and the devices on its selects
    LisaSpi& spi() { return spi_; }
    uint8_t get_port_c_out() const { return port_c_out_; }
    uint8_t get_port_c_dir() const { return port_c_dir_; }

private:
    // GPIO
    uint8_t port_a_in_;
    uint32_t cap_wave_ = 0, cap_cnt_ = 0;
    uint8_t port_b_out_;
    uint8_t port_c_out_;
    uint8_t port_c_in_;
    uint8_t port_c_dir_;
    uint8_t port_d_in_;
    uint8_t port_e_out_;
    uint8_t port_a_int_cfg0_;
    uint8_t port_a_int_cfg1_;
    uint8_t port_a_prev_;

    // Timers
    TimerState timer1_;
    TimerState timer2_;

    // Interrupts
    uint8_t int_enable_;
    uint8_t int_status_;          // the latched requests (timers, port, TTLC); the UARTs' are levels: uart_requests()
    uint8_t uart_requests() const;
    uint8_t uart_int_en_;
    bool    rev2_ = false;
    bool    rev3_ = false;
    uint8_t code_bank_ = 1, data_bank_ = 1, stack_bank_ = 1;
    uint8_t const_bank_ = 0;      // KB, 0x21F: reset 0, the lower half

    // I2C (simplified)
    uint16_t i2c_prescale_;
    uint8_t  i2c_ctrl_;
    uint8_t  i2c_rx_data_;
    uint8_t  i2c_status_;
    uint8_t  i2c_tx_data_;
    uint8_t  i2c_cmd_;

    // Connected UARTs
    LisaUart* uart1_;
    LisaUart* uart2_;

    // SPI master (the third chip)
    LisaSpi  spi_;
    // Timer 3 (the third chip, RESPIN 32)
    LisaTimer3 timer3_;
    // the WS2812B driver (the third chip, RESPIN 33)
    LisaWs2812 ws_;
    // the TTLC I/O window's writable bytes (RESPIN 30/31: TIDX 0x2C, TDATA
    // 0x2D): the output flops 0-5 and the storage 12-15 / 20-23, kept for
    // the WS2812B DMA's BASE / LEN (RESPIN 35); the inputs read 0
    uint8_t  tio_[24] = {};
    uint8_t  tidx_ = 0;
    uint8_t  spi_request() const { return rev3_ && spi_.irq() ? IntSrc::SPI : 0; }

    void tick_timer(TimerState& t, uint8_t int_bit);
};

#endif
