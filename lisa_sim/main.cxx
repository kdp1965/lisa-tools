#include "simulator.h"
#include "cli.h"

#include <cstdio>
#include <cstring>
#include <string>

static void usage(const char* prog) {
    printf("Usage: %s [options] [firmware.hex]\n", prog);
    printf("Options:\n");
    printf("  -h, --help       Show this help\n");
    printf("  -r, --run        Run immediately after loading\n");
    printf("  -t, --trace      Enable instruction trace\n");
    printf("  -b, --batch      Batch mode (run and exit)\n");
    printf("  -c N             Run for N cycles then stop\n");
    printf("  -s N             Run for N steps then stop\n");
    printf("  -p PORT          Connect UART1 to host serial port\n");
    printf("     --port PORT\n");
    printf("  -m BYTES         Data RAM size, a power of two (128 = the TT07 chip\n");
    printf("                   without the cache; default 32768); addresses wrap\n");
    printf("  --fixed-irq      Interrupt semantics of a fixed core: taken only between\n");
    printf("                   whole instructions (cond == 3, no ldx in flight), amode,\n");
    printf("                   cflag_save and signed_inversion shadowed. Default: the\n");
    printf("                   TT07 silicon, where an interrupt after an if or the first\n");
    printf("                   word of an ldx corrupts execution\n");
}

int main(int argc, char** argv) {
    LisaSimulator sim;

    std::string firmware_file;
    bool auto_run = false;
    bool batch = false;
    unsigned long ram_bytes = 0;
    bool fixed_irq = false;
    bool trace = false;
    uint64_t max_cycles = 0;
    int max_steps = 0;
    std::string serial_port;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--run") == 0) {
            auto_run = true;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0) {
            trace = true;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--batch") == 0) {
            batch = true;
            auto_run = true;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            max_cycles = strtoull(argv[++i], nullptr, 0);
            auto_run = true;
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            max_steps = atoi(argv[++i]);
            auto_run = true;
        } else if (strcmp(argv[i], "--fixed-irq") == 0) {
            fixed_irq = true;
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            ram_bytes = strtoul(argv[++i], nullptr, 0);
            if (ram_bytes < 2 || ram_bytes > 32768 || (ram_bytes & (ram_bytes - 1))) {
                fprintf(stderr, "Error: -m needs a power of two from 2 to 32768\n");
                return 1;
            }
        } else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) && i + 1 < argc) {
            serial_port = argv[++i];
        } else if (argv[i][0] != '-') {
            firmware_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    printf("LISA 8-bit Microcontroller Simulator v1.0\n");
    printf("=========================================\n");

    if (ram_bytes)
        sim.memory().set_data_size(ram_bytes);
    if (fixed_irq)
        sim.core().set_fixed_irq(true);

    if (!firmware_file.empty()) {
        if (sim.load_firmware(firmware_file)) {
            printf("Loaded %zu instruction words from '%s'\n",
                   sim.memory().firmware_words(), firmware_file.c_str());
            if (sim.cdb().loaded() && !batch)   // batch stdout is the program's output
                printf("Debug info from '%s'\n", sim.cdb().path().c_str());
        } else {
            fprintf(stderr, "Error: could not load '%s'\n", firmware_file.c_str());
            if (batch) return 1;
        }
    }

    if (trace) sim.set_trace(true);

    if (batch || auto_run) {
        // Echo UART1 output (and trace lines) to stdout when there is no CLI
        sim.set_output_callback([](const std::string& s) { fputs(s.c_str(), stdout); fflush(stdout); });
    }

    if (!serial_port.empty()) {
        if (sim.connect_uart_host(serial_port.c_str())) {
            printf("Connected UART1 to %s\n", serial_port.c_str());
        } else {
            fprintf(stderr, "Error: could not open serial port '%s'\n", serial_port.c_str());
            if (batch) return 1;
        }
    }

    if (auto_run) {
        if (max_steps > 0) {
            sim.step_n(max_steps);
        } else if (max_cycles > 0) {
            sim.run_cycles(max_cycles);
        } else {
            sim.run();
        }

        auto st = sim.get_state();
        printf("\nExecution complete: %llu instructions, %llu cycles\n",
               (unsigned long long)st.inst_count,
               (unsigned long long)st.cycle_count);
        printf("PC=%04X A=%02X IX=%04X SP=%04X Z=%d C=%d\n",
               st.pc, st.a, st.ix, st.sp, st.zflag, st.cflag);

        if (batch) return 0;
    }

    LisaCli cli(sim);
    cli.run();

    return 0;
}
