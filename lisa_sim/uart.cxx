#include "uart.h"
#include "serial.h"
#include <cstdio>

LisaUart::LisaUart()
    : host_serial_(nullptr)
{
    reset();
}

LisaUart::~LisaUart() {
    disconnect_host();
}

void LisaUart::reset() {
    tx_empty_ = true;
    while (!tx_out_.empty()) tx_out_.pop();
    while (!rx_buf_.empty()) rx_buf_.pop();
}

void LisaUart::write_tx(uint8_t data) {
    tx_out_.push(data);
    tx_empty_ = true;  // In functional sim, byte transmits instantly

    // Forward to host serial port if connected
    if (host_serial_) {
        ser_write_byte(host_serial_, (char)data);
    }
}

uint8_t LisaUart::read_rx() {
    if (rx_buf_.empty()) return 0;
    uint8_t val = rx_buf_.front();
    rx_buf_.pop();
    return val;
}

void LisaUart::inject_rx(uint8_t byte) {
    rx_buf_.push(byte);
}

uint8_t LisaUart::get_output() {
    if (tx_out_.empty()) return 0;
    uint8_t val = tx_out_.front();
    tx_out_.pop();
    return val;
}

uint8_t LisaUart::status() const {
    uint8_t st = 0;
    if (tx_empty_)          st |= 0x02;  // bit 1: tx_buf_empty
    if (!rx_buf_.empty())   st |= 0x01;  // bit 0: rx_data_avail
    return st;
}

bool LisaUart::connect_host(const char* port_name) {
    // Disconnect existing connection first
    disconnect_host();

    int err = ser_init(port_name, &host_serial_);
    if (err != SER_NO_ERROR || !host_serial_) {
        printf("Error: failed to open serial port '%s' (err=%d)\n", port_name, err);
        host_serial_ = nullptr;
        host_port_name_.clear();
        return false;
    }

    // Configure for raw 8N1 (matching typical UART defaults)
    ser_set_baud(host_serial_, 9600);
    ser_set_bit_size(host_serial_, 8);
    ser_set_parity(host_serial_, 'N');
    ser_set_stop_bits(host_serial_, 1);

    host_port_name_ = port_name;
    return true;
}

void LisaUart::disconnect_host() {
    if (host_serial_) {
        ser_close_port(host_serial_);
        ser_deinit(host_serial_);
        host_serial_ = nullptr;
        host_port_name_.clear();
    }
}

void LisaUart::poll_host() {
    if (!host_serial_)
        return;

    // Read bytes from host serial port -> inject into RX buffer
    char ch;
    while (ser_read_byte(host_serial_, &ch) == SER_NO_ERROR) {
        rx_buf_.push((uint8_t)ch);
    }
}
