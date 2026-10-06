#ifndef LISA_CLI_H
#define LISA_CLI_H

#include <string>
#include <vector>

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

    // Helpers
    std::vector<std::string> tokenize(const std::string& line);
    uint32_t parse_number(const std::string& s);
    uint32_t parse_addr(const std::string& s);  // Defaults to hex for addresses
    bool require_stopped(const char* cmd_name);
    static std::string get_history_path();
};

#endif
