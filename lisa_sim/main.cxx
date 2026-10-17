#include "simulator.h"
#include "cli.h"

#include <cstdio>
#include <cstring>
#include <cctype>
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
    printf("  --icache=N[xW][,P][,direct][,stream]  Model the instruction fetch: a cache of N lines\n");
    printf("                   of W words (the chip: 1x4; LRU, or direct-mapped aligned lines as a RAM\n");
    printf("                   macro), filled by lines of P clocks (default %d + %d per word), or with\n", InstCache::FILL_OVERHEAD, InstCache::FILL_PER_WORD);
    printf("                   stream by a QSPI burst that keeps running (%d per word, a restart of P,\n", InstCache::FILL_PER_WORD);
    printf("                   default %d, on a jump; N may be 0). The counts go to stderr after a run\n", InstCache::FILL_OVERHEAD + InstCache::FILL_PER_WORD);
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
    int icache_lines = -1, icache_words = 4, icache_penalty = 0;
    bool icache_direct = false, icache_stream = false;
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
        } else if (strncmp(argv[i], "--icache=", 9) == 0) {
            char* p = argv[i] + 9;
            icache_lines = (int)strtol(p, &p, 0);
            if (*p == 'x') icache_words = (int)strtol(p + 1, &p, 0);
            if (*p == ',' && isdigit((unsigned char)p[1])) icache_penalty = (int)strtol(p + 1, &p, 0);
            while (*p == ',') {
                if (strncmp(p + 1, "direct", 6) == 0) { icache_direct = true; p += 7; }
                else if (strncmp(p + 1, "stream", 6) == 0) { icache_stream = true; p += 7; }
                else break;
            }
            if (icache_lines < (icache_stream ? 0 : 1) || icache_words < 1 || *p) {
                fprintf(stderr, "Error: --icache=N[xW][,P][,direct][,stream] with W >= 1 and N >= 1 (0 with stream)\n");
                return 1;
            }
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
    if (icache_lines >= 0)
        sim.icache().configure(icache_lines, icache_words, icache_penalty, icache_direct, icache_stream);

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
        if (sim.icache().enabled())     // stderr: stdout is the program's output
            fputs(sim.icache().report().c_str(), stderr);

        if (batch) return 0;
    }

    LisaCli cli(sim);
    cli.run();

    return 0;
}
