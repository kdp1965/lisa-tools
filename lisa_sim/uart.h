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

    // The second chip's receiver (--rev2, RESPIN 14): an 8-byte FIFO fed one
    // byte per char_cycles (bytes from the host or the CLI wait in a queue,
    // as they would on the wire), a byte arriving with the FIFO full is
    // dropped and sets the overrun flag (status bit 2) until the FIFO has
    // been read empty.  depth 0 (the default, TT07): unbounded, instant
    void     set_rx_fifo(int depth, uint32_t char_cycles) { fifo_depth_ = depth; char_cycles_ = char_cycles; }
    void     advance(uint32_t cycles);  // simulated time passes: deliver queued bytes
    bool     rx_overrun() const { return overrun_; }

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
    std::queue<uint8_t> rx_buf_;  // Bytes received (the FIFO the program reads)
    std::queue<uint8_t> rx_wire_; // ... waiting their turn on the wire (--rev2 pacing)
    int      fifo_depth_ = 0;
    uint32_t char_cycles_ = 0;
    uint32_t wire_wait_ = 0;      // cycles until the next byte lands in the FIFO
    uint32_t tx_busy_ = 0;        // cycles until the byte being sent has left (tx_buf_empty again)
    bool     overrun_ = false;

    // Host serial port bridge
    ser_params_t* host_serial_;
    std::string   host_port_name_;
};

#endif
