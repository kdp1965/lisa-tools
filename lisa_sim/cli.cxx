#include "cli.h"
#include "simulator.h"
#include "disasm.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <algorithm>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

std::string LisaCli::get_history_path() {
    const char* home = getenv("HOME");
#ifdef _WIN32
    if (!home) home = getenv("USERPROFILE");
#endif
    if (!home) home = ".";
    return std::string(home) + "/.lisa_sim_history";
}

LisaCli::LisaCli(LisaSimulator& sim)
    : sim_(sim), quit_(false) {}

std::vector<std::string> LisaCli::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

uint32_t LisaCli::parse_number(const std::string& s) {
    if (s.empty()) return 0;
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        return (uint32_t)strtoul(s.c_str(), nullptr, 16);
    if (s.size() > 1 && s.back() == 'h')
        return (uint32_t)strtoul(s.c_str(), nullptr, 16);
    // If string contains a-f/A-F, treat as hex
    for (char c : s) {
        if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
            return (uint32_t)strtoul(s.c_str(), nullptr, 16);
    }
    return (uint32_t)strtoul(s.c_str(), nullptr, 10);
}

uint32_t LisaCli::parse_addr(const std::string& s) {
    if (s.empty()) return 0;
    // Addresses always parsed as hex (standard for embedded debuggers)
    return (uint32_t)strtoul(s.c_str(), nullptr, 16);
}

bool LisaCli::require_stopped(const char* cmd_name) {
    if (sim_.thread_active() && !sim_.is_paused()) {
        printf("Cannot '%s' while simulation is running (use 'halt' first)\n", cmd_name);
        return false;
    }
    return true;
}

void LisaCli::run() {
    printf("LISA Simulator CLI — type 'help' for commands\n");

#ifdef HAVE_READLINE
    using_history();
    stifle_history(500);
    std::string hist_path = get_history_path();
    read_history(hist_path.c_str());
#endif

    while (!quit_) {
#ifdef HAVE_READLINE
        char* line_raw = readline("lisa> ");
        if (!line_raw) { quit_ = true; break; }
        std::string line(line_raw);
        if (!line.empty()) add_history(line_raw);
        free(line_raw);
#else
        printf("lisa> ");
        fflush(stdout);
        char buf[1024];
        if (!fgets(buf, sizeof(buf), stdin)) { quit_ = true; break; }
        std::string line(buf);
        // Strip trailing newline
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
            line.pop_back();
#endif
        if (!line.empty())
            process_command(line);
    }

#ifdef HAVE_READLINE
    write_history(hist_path.c_str());
#endif
}

bool LisaCli::process_command(const std::string& line) {
    auto args = tokenize(line);
    if (args.empty()) return true;

    std::string cmd = args[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "help" || cmd == "h" || cmd == "?")        cmd_help();
    else if (cmd == "load" || cmd == "l")                  cmd_load(args);
    else if (cmd == "reset" || cmd == "rst")               cmd_reset();
    else if (cmd == "step" || cmd == "s")                  cmd_step(args);
    else if (cmd == "run" || cmd == "r")                   cmd_run(args);
    else if (cmd == "continue" || cmd == "c")              cmd_continue();
    else if (cmd == "halt")                                cmd_halt();
    else if (cmd == "status" || cmd == "st")               cmd_status();
    else if (cmd == "regs")                                cmd_regs();
    else if (cmd == "reg")                                 cmd_reg(args);
    else if (cmd == "mem" || cmd == "m")                   cmd_mem(args);
    else if (cmd == "memw" || cmd == "mw")                 cmd_memw(args);
    else if (cmd == "bp")                                  cmd_bp(args);
    else if (cmd == "bpd")                                 cmd_bpd(args);
    else if (cmd == "bpl")                                 cmd_bpl();
    else if (cmd == "bpc")                                 cmd_bpc();
    else if (cmd == "dis" || cmd == "d")                   cmd_dis(args);
    else if (cmd == "trace" || cmd == "t")                 cmd_trace(args);
    else if (cmd == "periph" || cmd == "p")                cmd_periph();
    else if (cmd == "uart" || cmd == "u")                  cmd_uart(args);
    else if (cmd == "serial" || cmd == "ser")              cmd_serial(args);
    else if (cmd == "bf16" || cmd == "f")                  cmd_bf16();
    else if (cmd == "info" || cmd == "i")                  cmd_info();
    else if (cmd == "quit" || cmd == "q" || cmd == "exit") {
        if (sim_.thread_active()) sim_.stop_async();
        quit_ = true; return false;
    }
    else printf("Unknown command: %s (type 'help')\n", cmd.c_str());

    return !quit_;
}

void LisaCli::cmd_help() {
    printf("Commands:\n");
    printf("  load <file>          Load firmware hex file\n");
    printf("  reset                Reset CPU and peripherals\n");
    printf("  step [N]             Step N instructions (default 1)\n");
    printf("  run [cycles]         Run in background (optionally for N cycles)\n");
    printf("  halt                 Pause background simulation\n");
    printf("  continue             Resume simulation after halt/breakpoint\n");
    printf("  status               Show simulation run state\n");
    printf("  regs                 Display all registers\n");
    printf("  reg <name> [val]     Read/write register (a,ix,sp,pc,ra,ia)\n");
    printf("  mem <addr> [len]     Dump data memory (default 16 bytes)\n");
    printf("  memw <addr> <val>    Write byte to data memory\n");
    printf("  bp <addr>            Set breakpoint\n");
    printf("  bpd <addr>           Delete breakpoint\n");
    printf("  bpl                  List breakpoints\n");
    printf("  bpc                  Clear all breakpoints\n");
    printf("  dis [addr] [count]   Disassemble (default at PC, 10 insns)\n");
    printf("  trace on|off         Toggle instruction trace\n");
    printf("  periph               Show peripheral state\n");
    printf("  uart <text>          Send text to UART1\n");
    printf("  serial <port> [1|2]  Connect UART to host serial port\n");
    printf("  serial off [1|2]     Disconnect host serial port\n");
    printf("  serial               Show host serial port status\n");
    printf("  bf16                 Show BF16 FPU registers\n");
    printf("  info                 Show simulator statistics\n");
    printf("  quit                 Exit\n");
}

void LisaCli::cmd_load(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printf("Usage: load <filename>\n");
        return;
    }
    if (sim_.thread_active()) {
        sim_.stop_async();
        printf("Simulation stopped\n");
    }
    if (sim_.load_firmware(args[1])) {
        printf("Loaded %zu instruction words from '%s'\n",
               sim_.memory().firmware_words(), args[1].c_str());
    } else {
        printf("Error: failed to load '%s'\n", args[1].c_str());
    }
}

void LisaCli::cmd_reset() {
    if (sim_.thread_active()) {
        sim_.stop_async();
    }
    sim_.reset();
    printf("Reset complete\n");
}

void LisaCli::cmd_step(const std::vector<std::string>& args) {
    if (!require_stopped("step")) return;

    int n = 1;
    if (args.size() > 1) n = (int)parse_number(args[1]);
    if (n <= 0) n = 1;

    sim_.core().resume();
    int total_cycles = 0;
    for (int i = 0; i < n; i++) {
        if (sim_.core().is_halted()) break;
        total_cycles += sim_.step();
    }

    auto st = sim_.get_state();
    uint16_t inst = sim_.memory().inst_read(st.pc);
    uint16_t next = sim_.memory().inst_read((st.pc + 1) & 0x7FFF);
    printf("PC=%04X  %04X  %-24s A=%02X IX=%04X SP=%04X Z=%d C=%d\n",
           st.pc, inst, lisa_disasm(inst, st.pc, next).c_str(),
           st.a, st.ix, st.sp, st.zflag, st.cflag);
    if (n > 1)
        printf("  (%d instructions, %d cycles)\n", n, total_cycles);
}

void LisaCli::cmd_run(const std::vector<std::string>& args) {
    if (sim_.thread_active() && !sim_.is_paused()) {
        printf("Simulation is already running (use 'halt' to pause)\n");
        return;
    }

    if (args.size() > 1) {
        uint64_t max_cycles = parse_number(args[1]);
        printf("Running for up to %llu cycles...\n", (unsigned long long)max_cycles);
        sim_.run_cycles_async(max_cycles);
    } else {
        printf("Running... (use 'halt' to pause)\n");
        sim_.run_async();
    }
}

void LisaCli::cmd_continue() {
    if (sim_.is_paused()) {
        printf("Resuming simulation...\n");
        sim_.resume_async();
        return;
    }
    if (sim_.thread_active()) {
        printf("Simulation is already running\n");
        return;
    }
    // Not in a thread — start a new async run
    printf("Continuing... (use 'halt' to pause)\n");
    sim_.run_async();
}

void LisaCli::cmd_halt() {
    if (!sim_.thread_active()) {
        printf("Simulation is not running\n");
        return;
    }
    if (sim_.is_paused()) {
        printf("Simulation is already paused\n");
        return;
    }
    sim_.pause();
    if (!sim_.thread_active()) {
        printf("Simulation has already stopped\n");
        return;
    }
    auto st = sim_.get_state();
    uint16_t inst = sim_.memory().inst_read(st.pc);
    uint16_t next = sim_.memory().inst_read((st.pc + 1) & 0x7FFF);
    printf("Paused at PC=%04X  %04X  %s\n", st.pc, inst,
           lisa_disasm(inst, st.pc, next).c_str());
    printf("  A=%02X IX=%04X SP=%04X Z=%d C=%d  (%llu instructions, %llu cycles)\n",
           st.a, st.ix, st.sp, st.zflag, st.cflag,
           (unsigned long long)st.inst_count, (unsigned long long)st.cycle_count);
}

void LisaCli::cmd_status() {
    auto st = sim_.get_state();
    if (sim_.thread_active()) {
        if (sim_.is_paused())
            printf("Status: PAUSED at PC=%04X (%llu instructions, %llu cycles)\n",
                   st.pc, (unsigned long long)st.inst_count,
                   (unsigned long long)st.cycle_count);
        else
            printf("Status: RUNNING (PC=%04X, %llu cycles)\n",
                   st.pc, (unsigned long long)st.cycle_count);
    } else {
        if (st.halted)
            printf("Status: HALTED at PC=%04X\n", st.pc);
        else
            printf("Status: IDLE at PC=%04X (%llu instructions, %llu cycles)\n",
                   st.pc, (unsigned long long)st.inst_count,
                   (unsigned long long)st.cycle_count);
    }
}

void LisaCli::cmd_regs() {
    auto st = sim_.get_state();
    printf("  A  = %02X (%3d)     IX = %04X (%5d)  IX.c = %d\n",
           st.a, st.a, st.ix, st.ix, st.ix_cond);
    printf("  SP = %04X (%5d)  PC = %04X (%5d)\n",
           st.sp, st.sp, st.pc, st.pc);
    printf("  RA = %04X (%5d)  IA = %04X (%5d)  RA.c = %d\n",
           st.ra, st.ra, st.ia, st.ia, st.ra_cond);
    printf("  Z=%d  C=%d  COND=%d  AMODE=%d  IE=%d\n",
           st.zflag, st.cflag, st.cond, st.amode, st.ie);
    printf("  Cycles: %llu  Instructions: %llu\n",
           (unsigned long long)st.cycle_count, (unsigned long long)st.inst_count);
}

void LisaCli::cmd_reg(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printf("Usage: reg <name> [value]\n");
        return;
    }
    std::string name = args[1];
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    auto st = sim_.get_state();

    if (args.size() >= 3) {
        if (!require_stopped("reg")) return;
        uint32_t val = parse_number(args[2]);
        if (name == "a")       st.a = val & 0xFF;
        else if (name == "ix") st.ix = val & 0x7FFF;
        else if (name == "sp") st.sp = val & 0x7FFF;
        else if (name == "pc") st.pc = val & 0x7FFF;
        else if (name == "ra") st.ra = val & 0x7FFF;
        else if (name == "ia") st.ia = val & 0x7FFF;
        else { printf("Unknown register: %s\n", name.c_str()); return; }
        sim_.core().set_state(st);
        printf("  %s = %04X\n", args[1].c_str(), val);
    } else {
        if (name == "a")       printf("  A  = %02X (%d)\n", st.a, st.a);
        else if (name == "ix") printf("  IX = %04X (%d)\n", st.ix, st.ix);
        else if (name == "sp") printf("  SP = %04X (%d)\n", st.sp, st.sp);
        else if (name == "pc") printf("  PC = %04X (%d)\n", st.pc, st.pc);
        else if (name == "ra") printf("  RA = %04X (%d)\n", st.ra, st.ra);
        else if (name == "ia") printf("  IA = %04X (%d)\n", st.ia, st.ia);
        else printf("Unknown register: %s\n", name.c_str());
    }
}

void LisaCli::cmd_mem(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printf("Usage: mem <addr> [length]\n");
        return;
    }
    uint16_t addr = parse_addr(args[1]) & 0x7FFF;
    int len = 16;
    if (args.size() > 2) len = (int)parse_number(args[2]);
    if (len <= 0) len = 16;
    if (len > 256) len = 256;

    for (int i = 0; i < len; i += 16) {
        printf("  %04X: ", (addr + i) & 0x7FFF);
        int row = std::min(16, len - i);
        for (int j = 0; j < row; j++) {
            printf("%02X ", sim_.memory().data_read((addr + i + j) & 0x7FFF));
        }
        // ASCII
        printf(" |");
        for (int j = 0; j < row; j++) {
            uint8_t ch = sim_.memory().data_read((addr + i + j) & 0x7FFF);
            printf("%c", (ch >= 0x20 && ch < 0x7F) ? ch : '.');
        }
        printf("|\n");
    }
}

void LisaCli::cmd_memw(const std::vector<std::string>& args) {
    if (!require_stopped("memw")) return;
    if (args.size() < 3) {
        printf("Usage: memw <addr> <value>\n");
        return;
    }
    uint16_t addr = parse_addr(args[1]) & 0x7FFF;
    uint8_t val = parse_addr(args[2]) & 0xFF;
    sim_.memory().data_write(addr, val);
    printf("  [%04X] = %02X\n", addr, val);
}

void LisaCli::cmd_bp(const std::vector<std::string>& args) {
    if (!require_stopped("bp")) return;
    if (args.size() < 2) {
        printf("Usage: bp <addr>\n");
        return;
    }
    uint16_t addr = parse_addr(args[1]) & 0x7FFF;
    if (sim_.core().add_breakpoint(addr))
        printf("Breakpoint set at %04X\n", addr);
    else
        printf("Error: max breakpoints reached (%d)\n", LisaCore::MAX_BREAKPOINTS);
}

void LisaCli::cmd_bpd(const std::vector<std::string>& args) {
    if (!require_stopped("bpd")) return;
    if (args.size() < 2) {
        printf("Usage: bpd <addr>\n");
        return;
    }
    uint16_t addr = parse_addr(args[1]) & 0x7FFF;
    if (sim_.core().remove_breakpoint(addr))
        printf("Breakpoint removed at %04X\n", addr);
    else
        printf("No breakpoint at %04X\n", addr);
}

void LisaCli::cmd_bpl() {
    uint16_t addrs[LisaCore::MAX_BREAKPOINTS];
    int n = sim_.core().get_breakpoints(addrs, LisaCore::MAX_BREAKPOINTS);
    if (n == 0) {
        printf("No breakpoints set\n");
    } else {
        printf("Breakpoints:\n");
        for (int i = 0; i < n; i++) {
            uint16_t inst = sim_.memory().inst_read(addrs[i]);
            uint16_t next = sim_.memory().inst_read((addrs[i] + 1) & 0x7FFF);
            printf("  [%d] %04X: %s\n", i, addrs[i], lisa_disasm(inst, addrs[i], next).c_str());
        }
    }
}

void LisaCli::cmd_bpc() {
    if (!require_stopped("bpc")) return;
    sim_.core().clear_breakpoints();
    printf("All breakpoints cleared\n");
}

void LisaCli::cmd_dis(const std::vector<std::string>& args) {
    auto st = sim_.get_state();
    uint16_t addr = st.pc;
    int count = 10;

    if (args.size() > 1) addr = parse_addr(args[1]) & 0x7FFF;
    if (args.size() > 2) count = (int)parse_number(args[2]);
    if (count <= 0) count = 10;

    for (int i = 0; i < count; i++) {
        uint16_t a = (addr + i) & 0x7FFF;
        uint16_t inst = sim_.memory().inst_read(a);
        uint16_t next = sim_.memory().inst_read((a + 1) & 0x7FFF);
        const char* marker = (a == st.pc) ? ">" : " ";
        printf("%s %04X: %04X  %s\n", marker, a, inst, lisa_disasm(inst, a, next).c_str());
    }
}

void LisaCli::cmd_trace(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printf("Trace is %s\n", sim_.trace_enabled() ? "ON" : "OFF");
        return;
    }
    if (!require_stopped("trace")) return;
    std::string val = args[1];
    std::transform(val.begin(), val.end(), val.begin(), ::tolower);
    if (val == "on" || val == "1") {
        sim_.set_trace(true);
        printf("Trace enabled\n");
    } else {
        sim_.set_trace(false);
        printf("Trace disabled\n");
    }
}

void LisaCli::cmd_periph() {
    auto& p = sim_.periph();
    printf("GPIO:\n");
    printf("  Port A (in):  %02X    Port B (out): %02X\n",
           p.read(PeriphReg::PORT_A_IN), p.get_port_b());
    printf("  Port C data:  %01X    Port C dir:   %01X\n",
           p.read(PeriphReg::PORT_C_DATA), p.get_port_c_dir());
    printf("  Port D (in):  %02X    Port E (out): %02X\n",
           p.read(PeriphReg::PORT_D_IN), p.get_port_e());
    printf("Timers:\n");
    printf("  T1: prediv=%04X div=%04X cnt=%02X ctrl=%02X\n",
           (uint16_t)(p.read(PeriphReg::TIM1_PREDIV_HI) << 8 | p.read(PeriphReg::TIM1_PREDIV_LO)),
           (uint16_t)(p.read(PeriphReg::TIM1_DIV_HI) << 8 | p.read(PeriphReg::TIM1_DIV_LO)),
           p.read(PeriphReg::TIM1_COUNT), p.read(PeriphReg::TIM1_CTRL));
    printf("  T2: prediv=%04X div=%04X cnt=%02X ctrl=%02X\n",
           (uint16_t)(p.read(PeriphReg::TIM2_PREDIV_HI) << 8 | p.read(PeriphReg::TIM2_PREDIV_LO)),
           (uint16_t)(p.read(PeriphReg::TIM2_DIV_HI) << 8 | p.read(PeriphReg::TIM2_DIV_LO)),
           p.read(PeriphReg::TIM2_COUNT), p.read(PeriphReg::TIM2_CTRL));
    printf("Interrupts:\n");
    printf("  Enable: %02X  Status: %02X  Active: %02X\n",
           p.read(PeriphReg::INT_ENABLE), p.read(PeriphReg::INT_STATUS),
           p.get_interrupts());
    printf("UART:\n");
    printf("  UART1 status: %02X    UART2 status: %02X\n",
           p.read(PeriphReg::UART1_STATUS), p.read(PeriphReg::UART2_STATUS));
}

void LisaCli::cmd_uart(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printf("Usage: uart <text>  — send text string to UART1 RX\n");
        return;
    }
    // Join all args after "uart" as the string
    std::string text;
    for (size_t i = 1; i < args.size(); i++) {
        if (i > 1) text += ' ';
        text += args[i];
    }
    for (char c : text) {
        sim_.uart_send((uint8_t)c, 1);
    }
    printf("Sent %zu bytes to UART1\n", text.size());
}

void LisaCli::cmd_serial(const std::vector<std::string>& args) {
    // No args: show status
    if (args.size() < 2) {
        printf("Host serial port status:\n");
        printf("  UART1: %s\n", sim_.uart1().host_connected()
               ? sim_.uart1().host_port_name().c_str() : "(not connected)");
        printf("  UART2: %s\n", sim_.uart2().host_connected()
               ? sim_.uart2().host_port_name().c_str() : "(not connected)");
#ifdef WIN32
        printf("Hint: Use Com2Com to create virtual COM port pairs\n");
        printf("  serial COM3       — connect UART1 to COM3\n");
#else
        printf("Hint: Use socat to create virtual serial port pairs:\n");
        printf("  socat -d -d pty,raw,echo=0 pty,raw,echo=0\n");
        printf("  serial /dev/pts/X — connect UART1 to one end\n");
        printf("  minicom -D /dev/pts/Y — connect terminal to the other\n");
#endif
        return;
    }

    std::string subcmd = args[1];
    std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::tolower);

    // Determine which UART port (default 1)
    int uart_port = 1;
    if (args.size() > 2) {
        int p = (int)parse_number(args[2]);
        if (p == 2) uart_port = 2;
    }

    if (subcmd == "off" || subcmd == "close" || subcmd == "disconnect") {
        sim_.disconnect_uart_host(uart_port);
        printf("UART%d host serial disconnected\n", uart_port);
        return;
    }

    // Treat the argument as a port name
    if (sim_.connect_uart_host(args[1].c_str(), uart_port)) {
        printf("UART%d connected to host serial port '%s'\n", uart_port, args[1].c_str());
    }
}

void LisaCli::cmd_bf16() {
    auto& f = sim_.bf16();
    printf("BF16 FPU:\n");
    printf("  FACC = %04X  (%.6g)\n", f.facc, BF16Unit::to_float(f.facc));
    for (int i = 0; i < 4; i++) {
        printf("  F%d   = %04X  (%.6g)\n", i, f.f[i], BF16Unit::to_float(f.f[i]));
    }
}

void LisaCli::cmd_info() {
    auto st = sim_.get_state();
    printf("Simulator Info:\n");
    printf("  Instructions: %llu\n", (unsigned long long)st.inst_count);
    printf("  Cycles:       %llu\n", (unsigned long long)st.cycle_count);
    if (st.inst_count > 0)
        printf("  CPI:          %.2f\n",
               (double)st.cycle_count / st.inst_count);
    printf("  Firmware:     %zu words loaded\n", sim_.memory().firmware_words());
    printf("  Trace:        %s\n", sim_.trace_enabled() ? "ON" : "OFF");
    if (sim_.thread_active()) {
        if (sim_.is_paused())
            printf("  State:        PAUSED\n");
        else
            printf("  State:        RUNNING (background)\n");
    } else {
        printf("  State:        %s\n", st.halted ? "HALTED" : "IDLE");
    }
}
