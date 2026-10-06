#ifndef LISA_UART_H
#define LISA_UART_H

#include <cstdint>
#include <string>
#include <queue>

struct ser_params;
typedef struct ser_params ser_params_t;

class LisaUart {
public:
    LisaUart();
    ~LisaUart();
    void reset();

    // Register interface (called by periph)
    void     write_tx(uint8_t data);
    uint8_t  read_rx();
    bool     tx_buf_empty() const { return tx_empty_; }
    bool     rx_data_avail() const { return !rx_buf_.empty(); }

    // External interface — inject bytes from host / retrieve output
    void     inject_rx(uint8_t byte);  // Host sends byte to UART RX
    bool     output_available() const { return !tx_out_.empty(); }
    uint8_t  get_output();             // Host reads byte from UART TX

    // Status byte for register read
    uint8_t  status() const;

    // Host serial port bridge
    bool     connect_host(const char* port_name);
    void     disconnect_host();
    bool     host_connected() const { return host_serial_ != nullptr; }
    const std::string& host_port_name() const { return host_port_name_; }
    void     poll_host();  // Read from host serial -> RX buf; TX queue -> host serial

private:
    // TX side
    bool     tx_empty_;
    std::queue<uint8_t> tx_out_;  // Bytes transmitted (host reads these)

    // RX side
    std::queue<uint8_t> rx_buf_;  // Bytes received (from host injection)

    // Host serial port bridge
    ser_params_t* host_serial_;
    std::string   host_port_name_;
};

#endif
