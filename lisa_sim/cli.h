#ifndef LISA_CLI_H
#define LISA_CLI_H

#include <string>
#include <vector>
#include "cdb.h"

class LisaSimulator;

class LisaCli {
public:
    LisaCli(LisaSimulator& sim);

    // Run the interactive CLI loop
    void run();

    // Process a single command, return false to quit
    bool process_command(const std::string& line);

private:
    LisaSimulator& sim_;
    bool           quit_;

    // Command handlers
    void cmd_help();
    void cmd_load(const std::vector<std::string>& args);
    void cmd_reset();
    void cmd_step(const std::vector<std::string>& args);
    void cmd_run(const std::vector<std::string>& args);
    void cmd_continue();
    void cmd_halt();
    void cmd_status();
    void cmd_regs();
    void cmd_reg(const std::vector<std::string>& args);
    void cmd_mem(const std::vector<std::string>& args);
    void cmd_memw(const std::vector<std::string>& args);
    void cmd_bp(const std::vector<std::string>& args);
    void cmd_bpd(const std::vector<std::string>& args);
    void cmd_bpl();
    void cmd_bpc();
    void cmd_dis(const std::vector<std::string>& args);
    void cmd_trace(const std::vector<std::string>& args);
    void cmd_periph();
    void cmd_uart(const std::vector<std::string>& args);
    void cmd_serial(const std::vector<std::string>& args);
    void cmd_bf16();
    void cmd_info();

    // Source-level commands (the .cdb sdcc --debug writes, see ../lisa_cdb)
    void cmd_break(const std::vector<std::string>& args);
    void cmd_list(const std::vector<std::string>& args);
    void cmd_next(bool into);
    void cmd_finish();
    void cmd_print(const std::vector<std::string>& args);
    void cmd_locals();
    void cmd_bt(const std::vector<std::string>& args);
    void cmd_cdb(const std::vector<std::string>& args);
    // the chip's way: frames unwound from the registers and the stack, a
    // line left by running to breakpoints - checked here against the
    // simulator's shadow call stack and stepping
    void cmd_exits(bool into);
    void cmd_nexthw(int mode);                  // 0 next, 1 into, 2 finish

    // Helpers
    std::vector<std::string> tokenize(const std::string& line);
    uint32_t parse_number(const std::string& s);
    uint32_t parse_addr(const std::string& s);  // Defaults to hex for addresses
    bool require_stopped(const char* cmd_name);
    static std::string get_history_path();

    // Source-level helpers
    void print_location();                      // the PC line, then file:line and the source
    void report_cdb();                          // what load found
    bool resolve_location(const std::string& spec, uint16_t& addr, std::string& what);
    bool frame_ra_saved(const lisa::CdbFunction& f, uint16_t pc);
    lisa::Cdb::Frame frame_of(size_t depth, uint16_t& pc, const lisa::CdbFunction** fn);
    std::string list_file_;                     // where `list` continues
    int list_line_ = 0;                         // the last line listed, 0: list around the PC
};

#endif
