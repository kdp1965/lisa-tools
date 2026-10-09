#ifndef LISA_SIMULATOR_H
#define LISA_SIMULATOR_H

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "core.h"
#include "memory.h"
#include "periph.h"
#include "uart.h"
#include "bf16.h"
#include "div.h"
#include "cdb.h"

class LisaSimulator {
public:
    LisaSimulator();
    ~LisaSimulator();

    // Lifecycle.  Loading firmware also loads the debug information sdcc
    // --debug writes beside it (<base>.cdb), if there is any.
    void reset();
    bool load_firmware(const std::string& filename);
    lisa::Cdb& cdb() { return cdb_; }
    const std::string& firmware_path() const { return firmware_path_; }

    // Execution control
    int  step();                    // Execute one instruction, return cycles
    int  step_n(int n);             // Execute N instructions, return total cycles
    void run();                     // Run until halt/breakpoint
    void run_cycles(uint64_t max);  // Run for at most max cycles
    void stop();                    // Request stop (for synchronous run)

    // Asynchronous execution (simulation runs in background thread)
    void run_async();               // Run in background thread
    void run_cycles_async(uint64_t max); // Run with cycle limit in background
    void pause();                   // Pause background simulation
    void resume_async();            // Resume paused simulation
    void stop_async();              // Stop background simulation and join thread

    bool is_running()     const { return running_.load(); }
    bool is_paused()      const { return paused_.load(); }
    bool thread_active()  const { return thread_active_.load(); }

    // Access sub-modules
    LisaCore&        core()   { return core_; }
    LisaMemory&      memory() { return memory_; }
    LisaPeripherals& periph() { return periph_; }
    LisaUart&        uart1()  { return uart1_; }
    LisaUart&        uart2()  { return uart2_; }
    BF16Unit&        bf16()   { return bf16_; }
    LisaDiv&         div()    { return div_; }

    // Convenience state access
    LisaCoreState get_state() const { return core_.get_state(); }

    // UART I/O for CLI
    void uart_send(uint8_t byte, int port = 1);
    bool uart_output_available(int port = 1) const;
    uint8_t uart_receive(int port = 1);

    // Host serial port bridge
    bool connect_uart_host(const char* port_name, int port = 1);
    void disconnect_uart_host(int port = 1);

    // Trace control
    void set_trace(bool on);
    bool trace_enabled() const { return trace_on_; }

    // Output callback (for UART output, trace, etc.)
    using OutputCallback = std::function<void(const std::string&)>;
    void set_output_callback(OutputCallback cb) { output_cb_ = cb; }

    // Called from the background thread when it stops at a breakpoint or
    // a brk (instead of the default "[Simulation halted ...]" line)
    using StopCallback = std::function<void()>;
    void set_stop_callback(StopCallback cb) { stop_cb_ = cb; }

private:
    LisaCore        core_;
    LisaMemory      memory_;
    LisaPeripherals periph_;
    LisaUart        uart1_;
    LisaUart        uart2_;
    BF16Unit        bf16_;
    LisaDiv         div_;
    lisa::Cdb       cdb_;
    std::string     firmware_path_;

    std::atomic<bool> running_{false};
    bool trace_on_;
    std::atomic<bool> stop_requested_{false};

    // Threading
    std::thread       sim_thread_;
    std::mutex        mtx_;
    std::condition_variable cv_;
    std::atomic<bool> paused_{false};
    std::atomic<bool> pause_requested_{false};
    std::atomic<bool> thread_active_{false};

    OutputCallback output_cb_;
    StopCallback   stop_cb_;

    void connect_modules();
    void check_uart_output();
    void sim_thread_func(uint64_t max_cycles);
    void join_thread();
};

#endif
