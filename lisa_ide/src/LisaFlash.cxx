// LisaFlash.cxx - programming the program flash through LISA's debugger
// (the LISA Commander's "via LISA" path): 64 KB block erases and 16-bit
// page programs on the debugger's QSPI port, one word per round trip with
// the flash status read in the same exchange, then a read-back verify.
// The round trip through the RP2040's pass-through is ~2.6 ms, so the
// exchanges wait for the reply instead of ReadReg's fixed delay.
#include "Lisa.h"
#include "image.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

extern int gLastWasCtrlC;

static double now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

// Send cmd (it ends with a register read) and wait for that read's reply:
// four hex digits then \n\r.  -1 on a timeout.
int CLisa::Exchange(const char *cmd, uint32_t& value, int timeoutMs)
{
    char        line[64];
    int         n = 0;
    char        ch;
    double      t0 = now_ms();

    m_Access.Acquire();
    for (const char *p = cmd; *p; p++)
        ser_write_byte(m_pSer, *p);
    for (;;)
    {
        ser_poll(m_pSer);
        bool got = false;
        while (ser_read_byte(m_pSer, &ch) == SER_NO_ERROR)
        {
            got = true;
            if (ch == '\r' && n >= 5 && line[n - 1] == '\n' && isxdigit((unsigned char) line[n - 5]) &&
                isxdigit((unsigned char) line[n - 4]) && isxdigit((unsigned char) line[n - 3]) &&
                isxdigit((unsigned char) line[n - 2]))
            {
                line[n - 1] = 0;
                value = strtoul(&line[n - 5], NULL, 16);
                m_Access.Release();
                return 0;
            }
            if (n < (int) sizeof(line) - 1)
                line[n++] = ch;
            else
            {
                memmove(line, line + 32, n - 32);           // keep the tail
                n -= 32;
                line[n++] = ch;
            }
        }
        if (now_ms() - t0 > timeoutMs)
            break;
        if (!got)
        {
            struct timespec tm = { 0, 200000 };            // 0.2 ms
            nanosleep(&tm, NULL);
        }
    }
    m_Access.Release();
    return -1;
}

// the flash's status register (the debugger's 0x22: a read status command)
bool CLisa::FlashIdle(int timeoutMs)
{
    uint32_t    st;
    double      t0 = now_ms();

    for (;;)
    {
        if (Exchange("r22\n", st) == 0 && !(st & 0xff))
            return true;
        if (now_ms() - t0 > timeoutMs)
            return false;
        usleep(2000);
    }
}

int CLisa::ProgramFlash(const std::vector<uint16_t>& words, uint32_t base)
{
    uint32_t    bytes = (uint32_t) words.size() * 2;
    uint32_t    st;
    double      t0 = now_ms();

    // the 64 KB blocks the image touches
    for (uint32_t a = base & ~0xffffu; a < base + bytes; a += 0x10000)
    {
        Printf("erasing the 64 KB block at 0x%06x", a);
        SetDebugAddress(a);
        WriteReg(0x21, 0x0006);                                 // write enable
        WriteReg(0x21, 0x01d8);                                 // block erase at the debug address
        if (!FlashIdle(15000))
        {
            Printf("the flash stays busy after the erase");
            return -1;
        }
    }
    // a word per exchange: write it (the address steps by 2), read the status
    SetDebugAddress(base);
    int lastPct = -1;
    for (size_t i = 0; i < words.size(); i++)
    {
        char cmd[24];
        snprintf(cmd, sizeof cmd, "w20%04x\nr22\n", words[i]);
        if (Exchange(cmd, st) != 0)
        {
            Printf("no answer to the write of word %zu (0x%06x)", i, (unsigned) (base + 2 * i));
            return -1;
        }
        if ((st & 0xff) && !FlashIdle(5000))
        {
            Printf("the flash stays busy at word %zu", i);
            return -1;
        }
        int pct = (int) ((i + 1) * 100 / words.size());
        if (pct / 25 != lastPct / 25 || i + 1 == words.size())
        {
            Printf("  programmed %zu of %zu words (%d%%)", i + 1, words.size(), pct);
            lastPct = pct;
        }
        if (gLastWasCtrlC)
        {
            gLastWasCtrlC = 0;
            Printf("interrupted: the flash is only partly programmed");
            return -1;
        }
    }
    Printf("programmed %zu words at 0x%06x in %.1f s", words.size(), base, (now_ms() - t0) / 1000);
    return 0;
}

// the number of words that differ (the first few reported); -1 if the
// debugger stops answering
int CLisa::VerifyFlash(const std::vector<uint16_t>& words, uint32_t base, bool quiet)
{
    uint32_t    v;
    int         bad = 0;

    SetDebugAddress(base);
    for (size_t i = 0; i < words.size(); i++)
    {
        if (Exchange("r20\n", v) != 0)
        {
            Printf("no answer reading word %zu", i);
            return -1;
        }
        if ((uint16_t) v != words[i])
        {
            if (++bad <= 8 && !quiet)
                Printf("  0x%06x (PC 0x%04zx): flash 0x%04x, image 0x%04x", (unsigned) (base + 2 * i), i, v, words[i]);
        }
    }
    SetDebugAddress(base);
    return bad;
}

// load <prog.ihx> [base]: halt, program, verify, reset the core to PC 0,
// then load the image and its .cdb for debugging.  base defaults to the
// program fetch base (setup); another base is programmed but not fetched.
int CLisa::Load(int argc, char* argv[])
{
    std::vector<uint16_t> words;
    std::string err;

    if (!m_Connected)
    {
        Printf("Please connect to the target first");
        return 1;
    }
    if (m_LisaHasUart)
    {
        Printf("LISA has the UART (the terminal): leave the terminal tab first");
        return 1;
    }
    if (!lisa::load_hex_image(argv[1], words, &err))
    {
        Printf("%s", err.c_str());
        return 1;
    }
    uint32_t base = argc > 2 ? strtoul(argv[2], NULL, 0) : m_Setup.lisa1_base;
    if (base & 0xff)
    {
        Printf("the base must be a multiple of 0x100");
        return 1;
    }
    if (!m_Setup.is_flash[m_Setup.dbg_cs])
    {
        Printf("the debugger's chip select (CS%d) is not set up as a flash: see setup", m_Setup.dbg_cs);
        return 1;
    }
    Printf("loading %s: %zu words at 0x%06x%s", argv[1], words.size(), base,
           base != m_Setup.lisa1_base ? " (not the program fetch base: it will not run from there)" : "");

    // the program is being replaced: an immediate halt is fine
    WriteReg(REG_STATUS, 1);
    WriteReg(REG_PC, 0);
    m_Running = 0;
    int ret = ProgramFlash(words, base);
    if (ret == 0)
    {
        int bad = VerifyFlash(words, base, false);
        if (bad == 0)
            Printf("verified: the flash matches %s", argv[1]);
        else if (bad > 0)
        {
            Printf("verify FAILED: %d word(s) differ", bad);
            ret = -1;
        }
        else
            ret = -1;
    }
    // back to the fetch base, the instruction cache invalidated, the core at 0
    SetDebugAddress(m_Setup.lisa1_base);
    WriteReg(0x1d, (m_Setup.cache_on ? 0x10 : 0x04) | 0x20 | (m_Setup.cache_map & 3));
    WriteUartString("t");
    usleep(20000);
    WriteUartString("\n");
    usleep(20000);
    WriteUartString("v");
    AwaitString("lisav1", 6, 50);
    m_Shift.clear();
    if (ret != 0)
        return 1;
    if (base != m_Setup.lisa1_base)
        return 0;
    // ready to debug: the image and its .cdb, the sources in tabs
    char *av[2] = { (char *) "debug", argv[1] };
    Debug(2, av);
    Printf("the core is reset and halted at 0: break, then run");
    return 0;
}

int CLisa::Verify(int argc, char* argv[])
{
    std::vector<uint16_t> words;
    std::string err;

    if (!m_Connected || m_LisaHasUart)
        return 1;
    if (!lisa::load_hex_image(argv[1], words, &err))
    {
        Printf("%s", err.c_str());
        return 1;
    }
    uint32_t base = argc > 2 ? strtoul(argv[2], NULL, 0) : m_Setup.lisa1_base;
    double t0 = now_ms();
    int bad = VerifyFlash(words, base, false);
    if (bad == 0)
        Printf("verified: the flash at 0x%06x matches %s (%zu words, %.1f s)", base, argv[1], words.size(), (now_ms() - t0) / 1000);
    else if (bad > 0)
        Printf("%d of %zu words differ", bad, words.size());
    SetDebugAddress(m_Setup.lisa1_base);
    return bad == 0 ? 0 : 1;
}

// vim: sw=4 ts=4 et
