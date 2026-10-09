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
    : sim_(sim), quit_(false) {
    sim_.set_stop_callback([this] {
        auto st = sim_.get_state();
        printf("\n[%s at PC=%04X]\n", sim_.core().is_breakpoint(st.pc) ? "Breakpoint" : "Halted", st.pc);
        print_location();
        fflush(stdout);
    });
}

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
    else if (cmd == "break" || cmd == "b")                 cmd_break(args);
    else if (cmd == "list")                                cmd_list(args);
    else if (cmd == "next" || cmd == "n")                  cmd_next(false);
    else if (cmd == "into" || cmd == "in")                 cmd_next(true);
    else if (cmd == "finish" || cmd == "fin")              cmd_finish();
    else if (cmd == "print" || cmd == "pr")                cmd_print(args);
    else if (cmd == "locals")                              cmd_locals();
    else if (cmd == "bt" || cmd == "where")                cmd_bt();
    else if (cmd == "cdb")                                 cmd_cdb(args);
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
    printf("Source level, with the <firmware>.cdb that sdcc --debug writes:\n");
    printf("  break <loc>          Breakpoint at file:line, line, function or *addr\n");
    printf("  list [loc]           Source around the PC or a location, then onwards\n");
    printf("  next                 Run to the next source line, over calls\n");
    printf("  into                 Run to the next source line, into calls\n");
    printf("  finish               Run until the current function returns\n");
    printf("  print <expr>         A variable, with *, &, [i], .m and ->m\n");
    printf("  locals               The current function's arguments and locals\n");
    printf("  bt                   Backtrace\n");
    printf("  cdb [file]           Show or load debug information\n");
}

void LisaCli::report_cdb() {
    const lisa::Cdb& cdb = sim_.cdb();
    if (!cdb.loaded()) return;
    int missing = 0;
    for (const std::string& f : cdb.files())
        if (cdb.source_lines(f) == 0) missing++;
    printf("Debug info from '%s': %zu functions, %zu source lines%s\n",
           cdb.path().c_str(), cdb.functions().size(), cdb.lines().size(),
           missing ? " (some sources not found beside it)" : "");
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
        report_cdb();
        list_file_.clear();
        list_line_ = 0;
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

    print_location();
    if (n > 1)
        printf("  (%d instructions, %d cycles)\n", n, total_cycles);
}

void LisaCli::print_location() {
    auto st = sim_.get_state();
    uint16_t inst = sim_.memory().inst_read(st.pc);
    uint16_t next = sim_.memory().inst_read((st.pc + 1) & 0x7FFF);
    printf("PC=%04X  %04X  %-24s A=%02X IX=%04X SP=%04X Z=%d C=%d\n",
           st.pc, inst, lisa_disasm(inst, st.pc, next).c_str(),
           st.a, st.ix, st.sp, st.zflag, st.cflag);
    const lisa::Cdb& cdb = sim_.cdb();
    if (!cdb.loaded()) return;
    const lisa::CdbFunction* f = cdb.function_at(st.pc);
    const lisa::CdbLine* l = cdb.line_at(st.pc);
    if (l) {
        std::string src = cdb.source(l->file, l->line);
        size_t b = src.find_first_not_of(" \t");
        printf("  %s:%d in %s():  %s\n", l->file.c_str(), l->line, f->name.c_str(),
               b == std::string::npos ? "" : src.c_str() + b);
        list_line_ = 0;                 // `list` starts around here again
    } else if (f) {
        printf("  in %s()\n", f->name.c_str());
    }
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
    printf("Paused after %llu instructions, %llu cycles\n",
           (unsigned long long)st.inst_count, (unsigned long long)st.cycle_count);
    print_location();
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
            printf("  [%d] %04X: %-24s", i, addrs[i], lisa_disasm(inst, addrs[i], next).c_str());
            const lisa::CdbLine* l = sim_.cdb().loaded() ? sim_.cdb().line_at(addrs[i]) : nullptr;
            if (l) printf("  %s:%d in %s()", l->file.c_str(), l->line, sim_.cdb().function_at(addrs[i])->name.c_str());
            printf("\n");
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

// ---- Source level: sdcc --debug's .cdb through ../lisa_cdb ----

static bool all_digits(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) if (!isdigit((unsigned char)c)) return false;
    return true;
}

// Did the function save RA before pc was reached?  Its sra is in the
// prologue, so look between the entry and pc (at most a few words in).
bool LisaCli::frame_ra_saved(const lisa::CdbFunction& f, uint16_t pc) {
    for (uint16_t a = f.addr; a < pc && a < f.addr + 16; a++)
        if ((sim_.memory().inst_read(a) & 0xFFFC) == 0xA160)      // sra
            return true;
    return false;
}

// The frame `depth` calls up from the halted one: its PC (an outer
// frame's is the return address into it), its function, and what print
// needs - the entry SP comes from the shadow call stack.
lisa::Cdb::Frame LisaCli::frame_of(size_t depth, uint16_t& pc, const lisa::CdbFunction** fn) {
    auto st = sim_.get_state();
    const auto& calls = sim_.core().call_stack();
    size_t n = calls.size();
    lisa::Cdb::Frame fr;
    pc = depth == 0 ? st.pc : calls[n - depth].ret_pc;
    fr.entry_sp = depth < n ? calls[n - 1 - depth].sp : 0x7fff;
    const lisa::CdbFunction* f = sim_.cdb().function_at(depth == 0 ? pc : pc - 1);
    if (f) {
        fr.function = f->name;
        fr.ra_saved = frame_ra_saved(*f, pc);
    }
    fr.a = st.a;
    fr.a_valid = depth == 0;
    if (fn) *fn = f;
    return fr;
}

// file:line, a line in the file last listed (or the PC's), a function, or
// *addr / 0xaddr
bool LisaCli::resolve_location(const std::string& spec, uint16_t& addr, std::string& what) {
    const lisa::Cdb& cdb = sim_.cdb();
    if (spec[0] == '*' || spec.rfind("0x", 0) == 0) {
        addr = parse_addr(spec[0] == '*' ? spec.substr(1) : spec.substr(2)) & 0x7FFF;
        const lisa::CdbLine* l = cdb.loaded() ? cdb.line_at(addr) : nullptr;
        what = l ? l->file + ":" + std::to_string(l->line) : "";
        return true;
    }
    if (!cdb.loaded()) {
        printf("No debug info loaded: sdcc --debug writes <firmware>.cdb beside the .ihx\n");
        return false;
    }
    std::string file;
    int line = 0;
    size_t colon = spec.rfind(':');
    if (colon != std::string::npos) {
        file = spec.substr(0, colon);
        line = atoi(spec.c_str() + colon + 1);
    } else if (all_digits(spec)) {
        line = atoi(spec.c_str());
        const lisa::CdbLine* here = cdb.line_at(sim_.get_state().pc);
        file = !list_file_.empty() ? list_file_ : here ? here->file : cdb.files().empty() ? "" : cdb.files()[0];
    } else {
        const lisa::CdbFunction* f = cdb.function(spec);
        if (!f || !f->has_addr) {
            printf("No function '%s'\n", spec.c_str());
            return false;
        }
        addr = f->addr;
        for (const lisa::CdbLine& l : cdb.lines())           // its first line: past the prologue
            if (l.addr >= f->addr && (!f->has_end || l.addr < f->end)) { addr = l.addr; break; }
        const lisa::CdbLine* l = cdb.line_at(addr);
        what = f->name + "()" + (l ? " at " + l->file + ":" + std::to_string(l->line) : "");
        return true;
    }
    std::vector<uint16_t> addrs = cdb.addrs_of_line(file, line);
    if (addrs.empty()) {
        const lisa::CdbLine* nl = cdb.next_line_with_code(file, line);
        if (!nl) {
            printf("No code at or after %s:%d\n", file.c_str(), line);
            return false;
        }
        file = nl->file;
        line = nl->line;
        addrs = cdb.addrs_of_line(file, line);
    }
    addr = addrs[0];
    what = file + ":" + std::to_string(line);
    return true;
}

void LisaCli::cmd_break(const std::vector<std::string>& args) {
    if (!require_stopped("break")) return;
    if (args.size() < 2) {
        printf("Usage: break <file:line | line | function | *addr>\n");
        return;
    }
    uint16_t addr;
    std::string what;
    if (!resolve_location(args[1], addr, what)) return;
    if (sim_.core().add_breakpoint(addr)) {
        printf("Breakpoint at %04X", addr);
        if (!what.empty()) printf(": %s", what.c_str());
        printf("\n");
    } else {
        printf("Error: max breakpoints reached (%d)\n", LisaCore::MAX_BREAKPOINTS);
    }
}

void LisaCli::cmd_list(const std::vector<std::string>& args) {
    const lisa::Cdb& cdb = sim_.cdb();
    if (!cdb.loaded()) {
        printf("No debug info loaded: sdcc --debug writes <firmware>.cdb beside the .ihx\n");
        return;
    }
    auto st = sim_.get_state();
    const lisa::CdbLine* here = cdb.line_at(st.pc);
    std::string file = list_file_;
    int start;
    if (args.size() > 1) {                                   // around a location
        uint16_t addr;
        std::string what;
        if (!resolve_location(args[1], addr, what)) return;
        const lisa::CdbLine* l = cdb.line_at(addr);
        if (!l) {
            printf("No source for %04X\n", addr);
            return;
        }
        file = l->file;
        start = l->line - 5;
    } else if (list_line_ <= 0 || file.empty()) {            // around the PC
        if (!here) {
            printf("No source line for PC=%04X\n", st.pc);
            return;
        }
        file = here->file;
        start = here->line - 5;
    } else {
        start = list_line_ + 1;                              // onwards
    }
    if (start < 1) start = 1;
    int n = cdb.source_lines(file);
    if (n == 0) {
        printf("Source '%s' not found beside %s\n", file.c_str(), cdb.path().c_str());
        return;
    }
    if (start > n) {
        printf("End of %s\n", file.c_str());
        return;
    }
    uint16_t bps[LisaCore::MAX_BREAKPOINTS];
    int nbp = sim_.core().get_breakpoints(bps, LisaCore::MAX_BREAKPOINTS);
    for (int i = start; i < start + 10 && i <= n; i++) {
        char mark = ' ';
        for (int b = 0; b < nbp; b++) {
            const lisa::CdbLine* bl = cdb.line_at(bps[b]);
            if (bl && bl->line == i && bl->file == file) mark = '*';
        }
        if (here && here->line == i && here->file == file) mark = '>';
        printf("%c%5d  %s\n", mark, i, cdb.source(file, i).c_str());
        list_line_ = i;
    }
    list_file_ = file;
}

// Run to the start of another source line.  `next` stays in this frame
// (a call runs to its return); `into` stops at the first line of a callee
// with debug info.  A loop back to the same line counts as a new line.
void LisaCli::cmd_next(bool into) {
    if (!require_stopped(into ? "into" : "next")) return;
    const lisa::Cdb& cdb = sim_.cdb();
    if (!cdb.loaded()) {
        printf("No debug info loaded: sdcc --debug writes <firmware>.cdb beside the .ihx\n");
        return;
    }
    auto st = sim_.get_state();
    const lisa::CdbLine* l0 = cdb.line_at(st.pc);
    size_t depth0 = sim_.core().call_stack().size();
    uint16_t prev_pc = st.pc;
    sim_.core().resume();
    long budget = 50000000;
    bool reached = false;
    while (budget-- > 0) {
        sim_.step();
        if (sim_.core().is_halted()) {                       // a breakpoint
            reached = true;
            break;
        }
        uint16_t pc = sim_.get_state().pc;
        size_t depth = sim_.core().call_stack().size();
        if (depth > depth0 && !into) continue;               // inside a call
        if (depth < depth0) {                                // returned: any line stops
            depth0 = depth;
            l0 = nullptr;
        }
        const lisa::CdbLine* l = cdb.line_starting(pc);
        if (l && (!l0 || l->line != l0->line || l->file != l0->file || pc <= prev_pc)) {
            reached = true;
            break;
        }
        prev_pc = pc;
    }
    if (!reached)
        printf("Gave up after 50M instructions without reaching another source line\n");
    print_location();
}

void LisaCli::cmd_finish() {
    if (!require_stopped("finish")) return;
    const auto& calls = sim_.core().call_stack();
    if (calls.empty()) {
        printf("No call to return from\n");
        return;
    }
    size_t depth0 = calls.size();
    const lisa::CdbFunction* f = sim_.cdb().loaded() ? sim_.cdb().function_at(sim_.get_state().pc) : nullptr;
    printf("Run till exit from %s\n", f ? (f->name + "()").c_str() : "the current call");
    sim_.core().resume();
    long budget = 50000000;
    bool reached = false;
    while (budget-- > 0) {
        sim_.step();
        if (sim_.core().is_halted() || sim_.core().call_stack().size() < depth0) {
            reached = true;
            break;
        }
    }
    if (!reached)
        printf("Gave up after 50M instructions: no return\n");
    print_location();
}

void LisaCli::cmd_print(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printf("Usage: print <expr>   e.g. print s, print arr[2], print p->x, print *p, print &g\n");
        return;
    }
    const lisa::Cdb& cdb = sim_.cdb();
    if (!cdb.loaded()) {
        printf("No debug info loaded: sdcc --debug writes <firmware>.cdb beside the .ihx\n");
        return;
    }
    std::string expr;
    for (size_t i = 1; i < args.size(); i++) expr += args[i];
    uint16_t pc;
    const lisa::CdbFunction* f;
    lisa::Cdb::Frame fr = frame_of(0, pc, &f);
    std::string err;
    std::string s = cdb.print(expr, f ? &fr : nullptr, &err,
                              [this](uint16_t a) { return sim_.memory().data_read(a & 0x7FFF); },
                              [this](uint16_t a) { return sim_.periph().read(a & 0x1FF); },
                              [this](uint16_t w) { return sim_.memory().inst_read(w & 0x7FFF); });
    if (s.empty())
        printf("%s\n", err.c_str());
    else
        printf("%s = %s\n", expr.c_str(), s.c_str());
}

void LisaCli::cmd_locals() {
    const lisa::Cdb& cdb = sim_.cdb();
    if (!cdb.loaded()) {
        printf("No debug info loaded: sdcc --debug writes <firmware>.cdb beside the .ihx\n");
        return;
    }
    uint16_t pc;
    const lisa::CdbFunction* f;
    lisa::Cdb::Frame fr = frame_of(0, pc, &f);
    if (!f) {
        printf("PC=%04X is not in a C function\n", pc);
        return;
    }
    auto rd = [this](uint16_t a) { return sim_.memory().data_read(a & 0x7FFF); };
    auto rdp = [this](uint16_t a) { return sim_.periph().read(a & 0x1FF); };
    auto rc = [this](uint16_t w) { return sim_.memory().inst_read(w & 0x7FFF); };
    printf("%s(), entry SP=%04X%s:\n", f->name.c_str(), fr.entry_sp, fr.ra_saved ? ", RA saved" : "");
    for (int pass = 0; pass < 2; pass++) {                   // arguments, then locals
        for (const lisa::CdbSymbol* s : cdb.locals(f->name)) {
            bool arg = (s->space == 'B' && s->offset >= 0) || s->space == 'R';
            if (arg != (pass == 0)) continue;
            if (s->name.rfind("sloc", 0) == 0) continue;     // the compiler's spill slots
            lisa::Cdb::Value v;
            printf("  %s%s = ", pass == 0 ? "arg " : "", s->name.c_str());
            if (cdb.variable(s->name, &fr, v))
                printf("%s   %s%s\n", cdb.format_value(v, rd, rdp, rc).c_str(), s->type.describe().c_str(),
                       s->space == 'R' ? " in A" : "");
            else
                printf("<no location>   %s\n", s->type.describe().c_str());
        }
    }
}

void LisaCli::cmd_bt() {
    const lisa::Cdb& cdb = sim_.cdb();
    const auto& calls = sim_.core().call_stack();
    auto rd = [this](uint16_t a) { return sim_.memory().data_read(a & 0x7FFF); };
    auto rdp = [this](uint16_t a) { return sim_.periph().read(a & 0x1FF); };
    auto rc = [this](uint16_t w) { return sim_.memory().inst_read(w & 0x7FFF); };
    for (size_t depth = 0; depth <= calls.size() && depth < 32; depth++) {
        uint16_t pc;
        const lisa::CdbFunction* f = nullptr;
        lisa::Cdb::Frame fr;
        if (cdb.loaded())
            fr = frame_of(depth, pc, &f);
        else
            pc = depth == 0 ? sim_.get_state().pc : calls[calls.size() - depth].ret_pc;
        if (depth > 0 && calls[calls.size() - depth].isr)
            printf("    <interrupt>\n");
        printf("#%-2zu %04X", depth, pc);
        if (!f) {
            printf("  ??\n");
            continue;
        }
        std::string params;
        for (const lisa::CdbSymbol* s : cdb.locals(f->name)) {
            if (!((s->space == 'B' && s->offset >= 0) || (s->space == 'R' && depth == 0))) continue;
            lisa::Cdb::Value v;
            if (!params.empty()) params += ", ";
            params += s->name + "=" + (cdb.variable(s->name, &fr, v) ? cdb.format_value(v, rd, rdp, rc) : "?");
        }
        const lisa::CdbLine* l = cdb.line_at(depth == 0 ? pc : pc - 1);
        printf("  %s(%s)", f->name.c_str(), params.c_str());
        if (l) printf(" at %s:%d", l->file.c_str(), l->line);
        printf("\n");
    }
}

void LisaCli::cmd_cdb(const std::vector<std::string>& args) {
    if (args.size() > 1) {
        std::string err;
        if (!sim_.cdb().load(args[1], &err)) {
            printf("Error: %s\n", err.c_str());
            return;
        }
        list_file_.clear();
        list_line_ = 0;
    }
    const lisa::Cdb& cdb = sim_.cdb();
    if (!cdb.loaded()) {
        printf("No debug info loaded: sdcc --debug writes <firmware>.cdb beside the .ihx\n");
        return;
    }
    report_cdb();
    for (const std::string& f : cdb.files())
        printf("  %s: %s\n", f.c_str(), cdb.source_lines(f) ? "found" : "not found");
}
