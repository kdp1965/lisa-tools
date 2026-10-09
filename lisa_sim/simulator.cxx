#include "simulator.h"
#include "disasm.h"
#include <cstdio>

LisaSimulator::LisaSimulator()
    : trace_on_(false)
{
    connect_modules();
    reset();
}

LisaSimulator::~LisaSimulator() {
    stop_async();
}

void LisaSimulator::connect_modules() {
    core_.set_memory(&memory_);
    core_.set_peripherals(&periph_);
    core_.set_bf16(&bf16_);
    core_.set_div(&div_);
    periph_.set_uart1(&uart1_);
    periph_.set_uart2(&uart2_);
}

void LisaSimulator::reset() {
    stop_async();
    core_.reset();
    memory_.reset();
    periph_.reset();
    uart1_.reset();
    uart2_.reset();
    bf16_.reset();
    div_.reset();
    running_ = false;
    stop_requested_ = false;
    connect_modules();
}

bool LisaSimulator::load_firmware(const std::string& filename) {
    if (!memory_.load_hex(filename))
        return false;
    firmware_path_ = filename;
    // the debug information beside it: firmware.ihx -> firmware.cdb
    std::string base = filename;
    size_t dot = base.find_last_of('.');
    if (dot != std::string::npos && dot > base.find_last_of('/') + 1)
        base = base.substr(0, dot);
    cdb_ = lisa::Cdb();
    cdb_.load(base + ".cdb");
    return true;
}

int LisaSimulator::step() {
    int cycles = core_.step();
    // Tick peripherals for each cycle
    for (int i = 0; i < cycles; i++) {
        periph_.tick();
    }
    // Propagate peripheral interrupts to core
    core_.set_int_enable(periph_.get_interrupts());
    // Poll host serial ports for incoming data
    uart1_.poll_host();
    uart2_.poll_host();
    check_uart_output();
    return cycles;
}

int LisaSimulator::step_n(int n) {
    int total = 0;
    for (int i = 0; i < n && !core_.is_halted() && !stop_requested_; i++) {
        total += step();
    }
    return total;
}

void LisaSimulator::run() {
    running_ = true;
    stop_requested_ = false;
    while (!core_.is_halted() && !stop_requested_) {
        step();
    }
    running_ = false;
}

void LisaSimulator::run_cycles(uint64_t max) {
    running_ = true;
    stop_requested_ = false;
    uint64_t count = 0;
    while (count < max && !core_.is_halted() && !stop_requested_) {
        count += step();
    }
    running_ = false;
}

void LisaSimulator::stop() {
    stop_requested_ = true;
}

// --- Asynchronous (threaded) execution ---

void LisaSimulator::join_thread() {
    if (sim_thread_.joinable()) {
        stop_requested_ = true;
        pause_requested_ = false;
        {
            std::lock_guard<std::mutex> lk(mtx_);
        }
        cv_.notify_all();
        sim_thread_.join();
        stop_requested_ = false;
    }
}

void LisaSimulator::sim_thread_func(uint64_t max_cycles) {
    uint64_t cycles = 0;

    while (!stop_requested_.load()) {
        if (pause_requested_.load()) {
            std::unique_lock<std::mutex> lk(mtx_);
            paused_ = true;
            cv_.notify_all();
            while (pause_requested_.load() && !stop_requested_.load()) {
                cv_.wait(lk);
            }
            paused_ = false;
            cv_.notify_all();
            if (stop_requested_.load()) break;
        }

        if (core_.is_halted()) break;

        cycles += step();

        if (max_cycles > 0 && cycles >= max_cycles) break;
    }

    running_ = false;
    thread_active_ = false;
    paused_ = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        cv_.notify_all();
    }

    // Report stop reason (unless user explicitly stopped)
    if (!stop_requested_.load()) {
        auto st = get_state();
        if (core_.is_halted()) {
            if (stop_cb_) {
                stop_cb_();
            } else {
                printf("\n[Simulation halted at PC=%04X]\n", st.pc);
                fflush(stdout);
            }
        } else if (max_cycles > 0 && cycles >= max_cycles) {
            printf("\n[Cycle limit reached at PC=%04X after %llu cycles]\n",
                   st.pc, (unsigned long long)st.cycle_count);
            fflush(stdout);
        }
    }
}

void LisaSimulator::run_async() {
    join_thread();
    core_.resume();
    stop_requested_ = false;
    pause_requested_ = false;
    paused_ = false;
    thread_active_ = true;
    running_ = true;
    sim_thread_ = std::thread(&LisaSimulator::sim_thread_func, this, 0);
}

void LisaSimulator::run_cycles_async(uint64_t max) {
    join_thread();
    core_.resume();
    stop_requested_ = false;
    pause_requested_ = false;
    paused_ = false;
    thread_active_ = true;
    running_ = true;
    sim_thread_ = std::thread(&LisaSimulator::sim_thread_func, this, max);
}

void LisaSimulator::pause() {
    if (!thread_active_.load()) return;
    pause_requested_ = true;
    std::unique_lock<std::mutex> lk(mtx_);
    cv_.wait(lk, [this] { return paused_.load() || !thread_active_.load(); });
}

void LisaSimulator::resume_async() {
    pause_requested_ = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
    }
    cv_.notify_all();
    // Wait for the sim thread to acknowledge the resume
    std::unique_lock<std::mutex> lk(mtx_);
    cv_.wait(lk, [this] { return !paused_.load() || !thread_active_.load(); });
}

void LisaSimulator::stop_async() {
    join_thread();
}

void LisaSimulator::set_trace(bool on) {
    trace_on_ = on;
    if (on) {
        core_.set_trace_callback([this](uint16_t pc, uint16_t inst, const LisaCoreState&) {
            char buf[128];
            uint16_t next = memory_.inst_read((pc + 1) & 0x7FFF);
            uint16_t sp;
            sp = core_.get_state().sp;
            std::string dis = lisa_disasm(inst, pc, next);
            snprintf(buf, sizeof(buf), "  %04X: %04X  %-24s A=%02X IX=%04X SP=%04X RA=%04X %c%c STK=%02X %02X %02X %02X",
                     pc, inst, dis.c_str(),
                     core_.get_state().a, core_.get_state().ix, sp, 
                     core_.get_state().ra,
                     core_.get_state().zflag ? 'Z' : '.', core_.get_state().cflag ? 'C' : '.',
                     memory_.data_read(sp),
                     memory_.data_read(sp+1),
                     memory_.data_read(sp+2),
                     memory_.data_read(sp+3)
                     );
            if (output_cb_) {
                output_cb_(std::string(buf) + "\n");
            } else {
                printf("%s\n", buf);
            }
        });
    } else {
        core_.set_trace_callback(nullptr);
    }
}

void LisaSimulator::uart_send(uint8_t byte, int port) {
    if (port == 2)
        uart2_.inject_rx(byte);
    else
        uart1_.inject_rx(byte);
}

bool LisaSimulator::uart_output_available(int port) const {
    if (port == 2)
        return uart2_.output_available();
    return uart1_.output_available();
}

uint8_t LisaSimulator::uart_receive(int port) {
    if (port == 2)
        return uart2_.get_output();
    return uart1_.get_output();
}

void LisaSimulator::check_uart_output() {
    // Forward UART1 output (only when not bridged to host serial)
    while (uart1_.output_available() && !uart1_.host_connected()) {
        uint8_t ch = uart1_.get_output();
        if (output_cb_) {
            char buf[2] = { (char)ch, 0 };
            output_cb_(buf);
        }
    }
}

bool LisaSimulator::connect_uart_host(const char* port_name, int port) {
    if (port == 2)
        return uart2_.connect_host(port_name);
    return uart1_.connect_host(port_name);
}

void LisaSimulator::disconnect_uart_host(int port) {
    if (port == 2)
        uart2_.disconnect_host();
    else
        uart1_.disconnect_host();
}
