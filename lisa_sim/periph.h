#ifndef LISA_PERIPH_H
#define LISA_PERIPH_H

#include <cstdint>
#include <functional>

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

    // Timer 2
    constexpr uint8_t TIM2_PREDIV_LO  = 0x18;
    constexpr uint8_t TIM2_PREDIV_HI  = 0x19;
    constexpr uint8_t TIM2_DIV_LO     = 0x1A;
    constexpr uint8_t TIM2_DIV_HI     = 0x1B;
    constexpr uint8_t TIM2_CTRL       = 0x1C;
    constexpr uint8_t TIM2_COUNT      = 0x1D;

    // I2C
    constexpr uint8_t I2C_PRESCALE_LO = 0x20;
    constexpr uint8_t I2C_PRESCALE_HI = 0x21;
    constexpr uint8_t I2C_CTRL        = 0x22;
    constexpr uint8_t I2C_RX_DATA     = 0x23;
    constexpr uint8_t I2C_STATUS      = 0x24;
    constexpr uint8_t I2C_TX_DATA     = 0x25;
    constexpr uint8_t I2C_CMD         = 0x26;
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

    // Register read/write (called by core when d_periph is set)
    uint8_t read(uint8_t addr);
    void write(uint8_t addr, uint8_t val);

    // Advance one clock cycle (for timers, etc.)
    void tick();

    // Get pending interrupt mask (for core to poll)
    uint8_t get_interrupts() const;

    // Connect UARTs
    void set_uart1(LisaUart* u) { uart1_ = u; }
    void set_uart2(LisaUart* u) { uart2_ = u; }

    // GPIO access for external simulation
    void set_port_a(uint8_t val) { port_a_in_ = val; }
    uint8_t get_port_b() const { return port_b_out_; }
    void set_port_d(uint8_t val) { port_d_in_ = val; }
    uint8_t get_port_e() const { return port_e_out_; }
    void set_port_c_input(uint8_t val) { port_c_in_ = val; }
    uint8_t get_port_c_out() const { return port_c_out_; }
    uint8_t get_port_c_dir() const { return port_c_dir_; }

private:
    // GPIO
    uint8_t port_a_in_;
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
    uint8_t int_status_;
    uint8_t uart_int_en_;

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

    void tick_timer(TimerState& t, uint8_t int_bit);
};

#endif
