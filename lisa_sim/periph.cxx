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

    i2c_prescale_ = 0;
    i2c_ctrl_ = 0;
    i2c_rx_data_ = 0;
    i2c_status_ = 0;
    i2c_tx_data_ = 0;
    i2c_cmd_ = 0;
}

uint8_t LisaPeripherals::read(uint8_t addr) {
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
    case PeriphReg::INT_STATUS:      return int_status_;

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

    default:
        return 0;
    }
}

void LisaPeripherals::write(uint8_t addr, uint8_t val) {
    switch (addr) {
    // GPIO
    case PeriphReg::PORT_B_OUT:      port_b_out_ = val; break;
    case PeriphReg::PORT_C_DATA:     port_c_out_ = val & 0x0F; break;
    case PeriphReg::PORT_C_DIR:      port_c_dir_ = val & 0x0F; break;
    case PeriphReg::PORT_E_OUT:      port_e_out_ = val; break;
    case PeriphReg::PORT_A_INT_CFG0: port_a_int_cfg0_ = val; break;
    case PeriphReg::PORT_A_INT_CFG1: port_a_int_cfg1_ = val; break;

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
        int_status_ &= ~val;  // Write 1 to clear
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
        uart_int_en_ = val;
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

void LisaPeripherals::tick() {
    tick_timer(timer1_, IntSrc::TIMER1);
    tick_timer(timer2_, IntSrc::TIMER2);

    // GPIO Port A edge detection
    uint8_t rising  = port_a_in_ & ~port_a_prev_;
    uint8_t falling = ~port_a_in_ & port_a_prev_;
    if ((rising & port_a_int_cfg0_) || (falling & port_a_int_cfg1_)) {
        int_status_ |= IntSrc::GPIO_A;
    }
    port_a_prev_ = port_a_in_;

    // UART interrupt generation
    if (uart1_) {
        if ((uart_int_en_ & 0x01) && uart1_->rx_data_avail())
            int_status_ |= IntSrc::UART1;
        if ((uart_int_en_ & 0x02) && uart1_->tx_buf_empty())
            int_status_ |= IntSrc::UART1;
    }
    if (uart2_) {
        if ((uart_int_en_ & 0x04) && uart2_->rx_data_avail())
            int_status_ |= IntSrc::UART2;
        if ((uart_int_en_ & 0x08) && uart2_->tx_buf_empty())
            int_status_ |= IntSrc::UART2;
    }
}

uint8_t LisaPeripherals::get_interrupts() const {
    return int_status_ & int_enable_;
}
