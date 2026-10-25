#include <cstdlib>
#include <cstdio>
#include "periph.h"
#include "uart.h"

LisaPeripherals::LisaPeripherals()
    : uart1_(nullptr), uart2_(nullptr)
{
    reset();
}

void LisaPeripherals::reset() {
    port_a_in_ = 0;
    port_b_out_ = 0;
    port_c_out_ = 0;
    port_c_in_ = 0;
    port_c_dir_ = 0;
    port_d_in_ = 0;
    port_e_out_ = 0;
    port_a_int_cfg0_ = 0;
    port_a_int_cfg1_ = 0;
    port_a_prev_ = 0;

    timer1_ = {};
    timer2_ = {};

    int_enable_ = 0;
    int_status_ = 0;
    uart_int_en_ = 0;
    code_bank_ = data_bank_ = stack_bank_ = 1;

    i2c_prescale_ = 0;
    i2c_ctrl_ = 0;
    i2c_rx_data_ = 0;
    i2c_status_ = 0;
    i2c_tx_data_ = 0;
    i2c_cmd_ = 0;
    spi_.reset();
    timer3_.reset();
    ws_.reset();
    misc_.reset();
    for (auto& b : tio_) b = 0;
    tidx_ = 0;
}

uint8_t LisaPeripherals::read(uint8_t addr) {
    if (rev3_ && addr >= 0x30 && addr <= 0x3F)       // Timer 3 (RESPIN 32)
        return timer3_.read(addr);
    if (rev3_ && addr >= 0x40 && addr <= 0x45)       // the WS2812B driver (RESPIN 33)
        return ws_.read(addr);
    if (rev3_ && misc_.handles(addr))                 // RESPIN 36
        return misc_.read(addr);
    if (rev3_ && addr == 0x2C) return tidx_;         // the TTLC I/O window (RESPIN 30)
    if (rev3_ && addr == 0x2D) {
        uint8_t v = tio_[tidx_];
        tidx_ = tidx_ == 23 ? 0 : (uint8_t)(tidx_ + 1);
        return v;
    }
    switch (addr) {
    // GPIO
    case PeriphReg::PORT_A_IN:       return port_a_in_;
    case PeriphReg::PORT_B_OUT:      return port_b_out_;
    case PeriphReg::PORT_C_DATA: {
        uint8_t val = 0;
        for (int i = 0; i < 4; i++) {
            if (port_c_dir_ & (1 << i))
                val |= (port_c_out_ & (1 << i));
            else
                val |= (port_c_in_ & (1 << i));
        }
        return val & 0x0F;
    }
    case PeriphReg::PORT_C_DIR:      return port_c_dir_ & 0x0F;
    case PeriphReg::PORT_D_IN:       return port_d_in_;
    case PeriphReg::PORT_E_OUT:      return port_e_out_;
    case PeriphReg::PORT_A_INT_CFG0: return port_a_int_cfg0_;
    case PeriphReg::PORT_A_INT_CFG1: return port_a_int_cfg1_;

    // Timer 1
    case PeriphReg::TIM1_PREDIV_LO:  return timer1_.prediv & 0xFF;
    case PeriphReg::TIM1_PREDIV_HI:  return (timer1_.prediv >> 8) & 0xFF;
    case PeriphReg::TIM1_DIV_LO:     return timer1_.divisor & 0xFF;
    case PeriphReg::TIM1_DIV_HI:     return (timer1_.divisor >> 8) & 0xFF;
    case PeriphReg::TIM1_CTRL:       return (timer1_.rollover ? 0x80 : 0) | (timer1_.enable ? 0x01 : 0);
    case PeriphReg::TIM1_COUNT:      return timer1_.counter;

    // Interrupts
    case PeriphReg::INT_ENABLE:      return int_enable_;
    case PeriphReg::INT_STATUS: {
        uint8_t st = int_status_;
        if (rev3_ && ws_.ie())                       // the WS2812B's free half in place of the TTLC
            st = (uint8_t)((st & ~IntSrc::TTLC) | (ws_.irq() ? IntSrc::TTLC : 0));
        return st | uart_requests() | spi_request();
    }

    // UART 1
    case PeriphReg::UART1_DATA:
        return uart1_ ? uart1_->read_rx() : 0;
    case PeriphReg::UART1_STATUS:
        return uart1_ ? uart1_->status() : 0x80;

    // UART 2
    case PeriphReg::UART2_DATA:
        return uart2_ ? uart2_->read_rx() : 0;
    case PeriphReg::UART2_STATUS:
        return uart2_ ? uart2_->status() : 0x80;
    case PeriphReg::UART_INT_EN:
        return uart_int_en_;

    // Timer 2
    case PeriphReg::TIM2_PREDIV_LO:  return timer2_.prediv & 0xFF;
    case PeriphReg::TIM2_PREDIV_HI:  return (timer2_.prediv >> 8) & 0xFF;
    case PeriphReg::TIM2_DIV_LO:     return timer2_.divisor & 0xFF;
    case PeriphReg::TIM2_DIV_HI:     return (timer2_.divisor >> 8) & 0xFF;
    case PeriphReg::TIM2_CTRL:       return (timer2_.rollover ? 0x80 : 0) | (timer2_.enable ? 0x01 : 0);
    case PeriphReg::TIM2_COUNT:      return timer2_.counter;

    // I2C
    case PeriphReg::I2C_PRESCALE_LO: return i2c_prescale_ & 0xFF;
    case PeriphReg::I2C_PRESCALE_HI: return (i2c_prescale_ >> 8) & 0xFF;
    case PeriphReg::I2C_CTRL:        return i2c_ctrl_;
    case PeriphReg::I2C_RX_DATA:     return i2c_rx_data_;
    case PeriphReg::I2C_STATUS:      return i2c_status_;
    case PeriphReg::I2C_TX_DATA:     return i2c_tx_data_;
    case PeriphReg::I2C_CMD:         return i2c_cmd_;

    // the banks: nothing there on TT07
    case PeriphReg::BANK_CODE:       return rev2_ ? code_bank_ : 0;
    case PeriphReg::BANK_DATA:       return rev2_ && !rev3_ ? data_bank_ : 0;
    case PeriphReg::BANK_STACK:      return rev2_ && !rev3_ ? stack_bank_ : 0;
    case PeriphReg::BANK_CONST:      return rev2_ && !rev3_ ? const_bank_ : 0;

    // the SPI master: the third chip's
    case PeriphReg::SPI_DATA:
    case PeriphReg::SPI_CTRL:
    case PeriphReg::SPI_DIV:
    case PeriphReg::SPI_STATUS:      return rev3_ ? spi_.read(addr) : 0;

    default:
        return 0;
    }
}

void LisaPeripherals::write(uint8_t addr, uint8_t val) {
    if (rev3_ && addr >= 0x30 && addr <= 0x3F) {     // Timer 3 (RESPIN 32)
        timer3_.write(addr, val);
        return;
    }
    if (rev3_ && addr >= 0x40 && addr <= 0x45) {     // the WS2812B driver (RESPIN 33)
        if (addr == 0x41)                            // DMA: BASE / LEN from the output flops
            ws_.frame(tio_[0] | tio_[1] << 8, (tio_[2] | tio_[3] << 8) & 0x1FFF);
        ws_.write(addr, val);
        return;
    }
    if (rev3_ && misc_.handles(addr)) {               // RESPIN 36
        misc_.write(addr, val);
        return;
    }
    if (rev3_ && addr == 0x2C) { tidx_ = val < 24 ? val : (uint8_t)(val % 24); return; }
    if (rev3_ && addr == 0x2D) {
        bool w = tidx_ < 6 || (tidx_ >= 12 && tidx_ < 16) || tidx_ >= 20;
        if (w) tio_[tidx_] = val;
        tidx_ = tidx_ == 23 ? 0 : (uint8_t)(tidx_ + 1);
        return;
    }
    switch (addr) {
    // GPIO
    case PeriphReg::PORT_B_OUT:      port_b_out_ = val; break;
    case PeriphReg::PORT_C_DATA:     port_c_out_ = val & 0x0F; break;
    case PeriphReg::PORT_C_DIR:      port_c_dir_ = val & 0x0F; break;
    case PeriphReg::PORT_E_OUT:      port_e_out_ = val; break;
    case PeriphReg::PORT_A_INT_CFG0: port_a_int_cfg0_ = val; break;
    case PeriphReg::PORT_A_INT_CFG1: port_a_int_cfg1_ = val; break;
    case PeriphReg::BANK_CODE:       if (rev2_) code_bank_  = val & (rev3_ ? 0x7F : 0x3F); break;     // 6 bits: 64 code banks
    case PeriphReg::BANK_DATA:       if (rev2_ && !rev3_) data_bank_  = val & 0x1F; break;     // 5 bits: 32 granules of 8K (RESPIN 28)
    case PeriphReg::BANK_STACK:      if (rev2_ && !rev3_) stack_bank_ = val & 0x0F; break;
    case PeriphReg::BANK_CONST:      if (rev2_ && !rev3_) const_bank_ = val & 0x3F; break;     // KB (RESPIN 26)
    case PeriphReg::SPI_DATA:
    case PeriphReg::SPI_CTRL:
    case PeriphReg::SPI_DIV:
    case PeriphReg::SPI_STATUS:      if (rev3_) spi_.write(addr, val); break;

    // Timer 1
    case PeriphReg::TIM1_PREDIV_LO:
        timer1_.prediv = (timer1_.prediv & 0xFF00) | val;
        break;
    case PeriphReg::TIM1_PREDIV_HI:
        timer1_.prediv = (timer1_.prediv & 0x00FF) | ((uint16_t)val << 8);
        break;
    case PeriphReg::TIM1_DIV_LO:
        timer1_.divisor = (timer1_.divisor & 0xFF00) | val;
        break;
    case PeriphReg::TIM1_DIV_HI:
        timer1_.divisor = (timer1_.divisor & 0x00FF) | ((uint16_t)val << 8);
        break;
    case PeriphReg::TIM1_CTRL:
        timer1_.enable = (val & 0x01) != 0;
        if (val & 0x80) timer1_.rollover = false;  // Clear rollover on write
        break;
    case PeriphReg::TIM1_COUNT:
        timer1_.counter = val;
        break;

    // Interrupts
    case PeriphReg::INT_ENABLE:
        int_enable_ = val;
        break;
    case PeriphReg::INT_STATUS:
        int_status_ &= ~(val & ~(IntSrc::UART1 | IntSrc::UART2));  // Write 1 to clear a latched request; the UARTs' are levels
        break;

    // UART 1
    case PeriphReg::UART1_DATA:
        if (uart1_) uart1_->write_tx(val);
        break;

    // UART 2
    case PeriphReg::UART2_DATA:
        if (uart2_) uart2_->write_tx(val);
        break;
    case PeriphReg::UART_INT_EN:
        // TT07's lisa_periph.v never loaded these flops (they read 0; no
        // UART interrupt ever fired there); the second chip's do (RESPIN 25)
        if (rev2_) uart_int_en_ = val;
        break;

    // Timer 2
    case PeriphReg::TIM2_PREDIV_LO:
        timer2_.prediv = (timer2_.prediv & 0xFF00) | val;
        break;
    case PeriphReg::TIM2_PREDIV_HI:
        timer2_.prediv = (timer2_.prediv & 0x00FF) | ((uint16_t)val << 8);
        break;
    case PeriphReg::TIM2_DIV_LO:
        timer2_.divisor = (timer2_.divisor & 0xFF00) | val;
        break;
    case PeriphReg::TIM2_DIV_HI:
        timer2_.divisor = (timer2_.divisor & 0x00FF) | ((uint16_t)val << 8);
        break;
    case PeriphReg::TIM2_CTRL:
        timer2_.enable = (val & 0x01) != 0;
        if (val & 0x80) timer2_.rollover = false;
        break;
    case PeriphReg::TIM2_COUNT:
        timer2_.counter = val;
        break;

    // I2C
    case PeriphReg::I2C_PRESCALE_LO:
        i2c_prescale_ = (i2c_prescale_ & 0xFF00) | val;
        break;
    case PeriphReg::I2C_PRESCALE_HI:
        i2c_prescale_ = (i2c_prescale_ & 0x00FF) | ((uint16_t)val << 8);
        break;
    case PeriphReg::I2C_CTRL:
        i2c_ctrl_ = val;
        break;
    case PeriphReg::I2C_TX_DATA:
        i2c_tx_data_ = val;
        break;
    case PeriphReg::I2C_CMD:
        i2c_cmd_ = val;
        // Simplified: complete I2C transaction immediately
        i2c_status_ |= 0x01;  // Set IRQ flag (command complete)
        // LISA_I2C_TRACE=1: each byte on the bus to stderr - S / P the start
        // and stop, W the byte written, R a read (no device: it reads 0)
        if (getenv("LISA_I2C_TRACE") && (val & 0x30))
            fprintf(stderr, "i2c %s%c %02x%s\n", (val & 0x80) ? "S " : "", (val & 0x20) ? 'R' : 'W',
                    (val & 0x20) ? 0 : i2c_tx_data_, (val & 0x40) ? " P" : "");
        break;

    default:
        break;
    }
}

void LisaPeripherals::tick_timer(TimerState& t, uint8_t int_bit) {
    if (!t.enable) return;

    t.pre_counter++;
    if (t.pre_counter >= t.prediv) {
        t.pre_counter = 0;
        if (t.counter == 0) {
            t.counter = t.divisor & 0xFF;
            t.rollover = true;
            int_status_ |= int_bit;
        } else {
            t.counter--;
        }
    }
}

void LisaPeripherals::reset_for_watchdog() {
    LisaMisc36 keep = misc_;
    reset();
    misc_ = keep;
    misc_.reset_peripherals();
}

void LisaPeripherals::tick() {
    if (cap_wave_ && ++cap_cnt_ >= cap_wave_) {
        cap_cnt_ = 0;
        port_a_in_ ^= 0x30;
    }
    if (rev3_) {
        spi_.tick();
        timer3_.tick(port_a_in_);
        ws_.tick();
        misc_.tick();
    }
    tick_timer(timer1_, IntSrc::TIMER1);
    tick_timer(timer2_, IntSrc::TIMER2);

    // GPIO Port A edge detection
    uint8_t rising  = port_a_in_ & ~port_a_prev_;
    uint8_t falling = ~port_a_in_ & port_a_prev_;
    if ((rising & port_a_int_cfg0_) || (falling & port_a_int_cfg1_)) {
        int_status_ |= IntSrc::GPIO_A;
    }
    port_a_prev_ = port_a_in_;

    // the UART requests are levels, not latched here: uart_requests()
}

// The UARTs' interrupt requests, as lisa_periph.v since RESPIN 25 (the
// second chip, 2026-10-18): a byte waits and the receive interrupt is
// enabled, or the transmitter is empty and its interrupt is enabled - a
// level that lasts until the handler drains the FIFO, writes the next byte
// or drops the enable; a write to INT_STATUS does not clear it.  (TT07
// latched an edge, and its enables could not be written at all: no UART
// interrupt ever fired there - see the write of UART_INT_EN.)
uint8_t LisaPeripherals::uart_requests() const {
    uint8_t r = 0;
    if (uart1_ && (((uart_int_en_ & 0x01) && uart1_->rx_data_avail()) ||
                   ((uart_int_en_ & 0x02) && uart1_->tx_buf_empty())))
        r |= IntSrc::UART1;
    if (uart2_ && (((uart_int_en_ & 0x04) && uart2_->rx_data_avail()) ||
                   ((uart_int_en_ & 0x08) && uart2_->tx_buf_empty())))
        r |= IntSrc::UART2;
    return r;
}

uint16_t LisaPeripherals::get_interrupts() const {
    uint8_t st = int_status_;
    if (rev3_ && ws_.ie())                           // the WS2812B's free half takes the TTLC's source
        st = (uint8_t)((st & ~IntSrc::TTLC) | (ws_.irq() ? IntSrc::TTLC : 0));
    uint16_t r = (st | uart_requests() | spi_request()) & int_enable_;
    if (rev3_ && timer3_.irq())
        r |= IntSrc::TIMER3;
    return r;
}
