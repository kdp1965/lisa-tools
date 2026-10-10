// LisaCdb.cxx - source-level debugging on the chip from sdcc --debug's
// .cdb (the reader in ../lisa_cdb, shared with lisa_sim): the program
// image and its sources in tabs, breakpoints by file:line, next/into/
// finish by running to breakpoints (a UART round trip is ~30 ms and the
// TT07 step bit is unreliable, so a line is not stepped through: its exits
// are computed from the image and the four hardware breakpoints go there),
// and print/locals/bt from the registers and the stack - the frame rule
// SP = entry SP - the prologue's displacement holds at statement
// boundaries, which is where these commands stop.
#include "Lisa.h"
#include "image.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <time.h>
#include <algorithm>

extern int gLastWasCtrlC;

static const char *basename_of(const char *p)
{
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

/*
==============================================================================
The chip
==============================================================================
*/

// n bytes of data RAM from addr, through IX: with the core halted a
// debugger data read returns the byte at IX (see lisa-test/web); IX is put
// back and the debugger's data address left at 0 afterwards.
int CLisa::ReadRam(uint16_t addr, uint8_t *buf, int n)
{
    uint32_t    ix, v;
    int         got = 0;

    if (ReadReg(REG_IX, ix) == -1)
        return -1;
    for (int i = 0; i < n; i++)
    {
        WriteReg(REG_IX, (addr + i) & 0x7fff);
        if (ReadReg(6, v) == -1)
            break;
        buf[got++] = v & 0xff;
    }
    WriteReg(REG_IX, ix);
    WriteReg(7, 0);
    return got;
}

bool CLisa::ReadCore(uint32_t& pc, uint32_t& sp, uint32_t& ra, uint32_t& ix, uint32_t& acc)
{
    return ReadReg(REG_PC, pc) != -1 && ReadReg(REG_SP, sp) != -1 && ReadReg(REG_RA, ra) != -1 &&
           ReadReg(REG_IX, ix) != -1 && ReadReg(REG_ACC, acc) != -1;
}

// an instruction word: from the image `debug` loaded, else from the flash
// through the debugger's QSPI port
uint16_t CLisa::CodeWord(uint16_t pc)
{
    uint32_t    v = 0;

    pc &= 0x7fff;
    if (pc < m_Program.size())
        return m_Program[pc];
    SetDebugAddress(m_Setup.lisa1_base + pc * 2);
    ReadReg(0x20, v);
    return (uint16_t) v;
}

// Where the instruction op at pc can continue (TT07 decode, lisa_core.v):
// the taken and the fall-through address both, a predicated instruction
// may be skipped altogether.
std::vector<uint16_t> CLisa::NextPCs(uint16_t op, uint16_t pc, uint16_t ra, uint16_t ix)
{
    uint16_t    seq = (pc + 1) & 0x7fff;
    int         rel = op & 0x7ff;

    if (rel & 0x400)
        rel -= 0x800;
    uint16_t    relpc = (pc + rel) & 0x7fff;
    if (!(op & 0x8000))
        return { (uint16_t) (op & 0x7fff), seq };                               // jal
    uint16_t t5 = op >> 11, t6 = op >> 10, t9 = op >> 7, t10 = op >> 6, t11 = op >> 5;
    if (t5 == 0x16 || t5 == 0x15 || t5 == 0x17)
        return { relpc, seq };                                                  // br / bnz / bz
    if (t6 == 0x23 || t9 == 0x114 || t10 == 0x22c || t10 == 0x22e)
        return { ra, seq };                                                     // ret #k / ret / rc / rz
    if (t10 == 0x22d)
        return { seq };                                                         // rets: IA is not readable
    if (t11 == 0x454 || t11 == 0x455)
        return { ix, seq };                                                     // call ix / jmp ix
    if ((op >> 4) == 0xa18)
        return { (uint16_t) ((pc + 2) & 0x7fff) };                              // ldx: the next word is its operand
    return { seq };
}

// Poll the status register until the core halts; Ctrl-C halts it.
bool CLisa::WaitHalt(int timeoutMs)
{
    uint32_t    st;

    for (int t = 0; t < timeoutMs; t += 40)
    {
        if (ReadReg(REG_STATUS, st) != -1 && (st & 1))
            return true;
        if (gLastWasCtrlC)
        {
            gLastWasCtrlC = 0;
            WriteReg(REG_STATUS, 1);
            Printf("interrupted");
            return ReadReg(REG_STATUS, st) != -1 && (st & 1);
        }
    }
    return false;
}

// The UART is the program's while the core runs under next/into/finish
// (its putc would otherwise wait forever for a TX-ready that never comes)
// and the debugger's whenever the core is looked at: +++ with guard time
// either side takes it back (the RTL needs ~42 ms at 50 MHz), 'l' hands
// it over.  The watch window's polling pauses meanwhile: its reads would
// land in the program's input.
void CLisa::Grant(void)
{
    m_pTui->m_WatchPaused = 1;
    WriteUartString("l");
    m_LisaHasUart = true;
}

void CLisa::Reclaim(void)
{
    usleep(150000);
    WriteUartString("+++");
    usleep(150000);
    WriteUartString("\n");
    usleep(20000);
    m_LisaHasUart = false;
    WriteUartString("v");
    AwaitString("lisav1", 6, 50);
    m_pTui->m_WatchPaused = 0;
}

// The core is running: give the program the UART, look every so often
// whether the core has halted (taking the UART back to ask), and show what
// the program prints meanwhile, a line at a time.  Ctrl-C halts the core
// (an asynchronous halt can land right after a store and hit the TT07
// breakpoint hazard too - nothing to be done about that one).
bool CLisa::RunGranted(int timeoutMs)
{
    uint32_t    st;
    char        ch;
    bool        halted = false;

    for (long t = 0; t < timeoutMs; )
    {
        Grant();
        struct timespec tm = { 0, 10000000 };
        for (int i = 0; i < 20; i++)                            // 200 ms of the program's output
        {
            nanosleep(&tm, NULL);
            m_Access.Acquire();
            ser_poll(m_pSer);
            while (ser_read_byte(m_pSer, &ch) == SER_NO_ERROR)
            {
                if (ch == 0 || ch == -1 || ch == '\r')
                    continue;
                if (ch == '\n')
                {
                    m_Access.Release();
                    Printf("[LISA] %s", m_OutLine.c_str());
                    m_Access.Acquire();
                    m_OutLine.clear();
                }
                else
                    m_OutLine += ch;
            }
            m_Access.Release();
        }
        Reclaim();
        t += 600;
        if (ReadReg(REG_STATUS, st) != -1 && (st & 1))
        {
            halted = true;
            break;
        }
        if (gLastWasCtrlC)
        {
            gLastWasCtrlC = 0;
            Printf("interrupted");
            halted = SafeHalt();
            break;
        }
    }
    if (!m_OutLine.empty())
    {
        Printf("[LISA] %s", m_OutLine.c_str());
        m_OutLine.clear();
    }
    return halted;
}

// Halt a running core without the TT07 breakpoint hazard: an asynchronous
// halt can land right after a store and lose it.  A program being
// interrupted is nearly always in a loop, and PC can be read while the core
// runs, so: sample PC, plant breakpoints at safe addresses on the path it is
// running - the start of the source line it is on first (frames are exact
// there), then the sampled addresses - and let it run into one.  Only when
// none is hit (not a loop, or nothing safe in it) the asynchronous halt.
// The breakpoint registers are left for the caller to restore.
bool CLisa::SafeHalt(void)
{
    uint32_t    pc, st;
    auto        rc = [this](uint16_t w) -> uint16_t { return CodeWord(w); };
    std::vector<uint16_t> samples, cand;

    if (ReadReg(REG_STATUS, st) != -1 && (st & 1))
        return true;
    for (int i = 0; i < 8; i++)
        if (ReadReg(REG_PC, pc) != -1 && std::find(samples.begin(), samples.end(), (uint16_t) pc) == samples.end())
            samples.push_back(pc);
    auto add = [&](uint16_t x) {
        int     delta = 0;
        bool    ok = true;
        uint16_t s = lisa::Cdb::safe_stop(x, rc, &delta, &ok);
        if (!ok || cand.size() >= 4 || std::find(cand.begin(), cand.end(), s) != cand.end())
            return;
        if (s != x)
            m_Shift[s] = std::make_pair(x, delta);
        cand.push_back(s);
    };
    if (m_Cdb.loaded())
        for (uint16_t p : samples)
            if (const lisa::CdbLine *l = m_Cdb.line_at(p))
                add(l->addr);
    for (uint16_t p : samples)
        add(p);
    if (!cand.empty())
    {
        for (int i = 0; i < 4; i++)
            WriteReg(8 + i, i < (int) cand.size() ? (0x8000 | cand[i]) : 0);
        for (int i = 0; i < 20; i++)                            // up to a second for the loop to come round
        {
            if (ReadReg(REG_STATUS, st) != -1 && (st & 1))
                return true;
            usleep(20000);
        }
    }
    Printf("warning: the program is not looping through a safe address (sampled %zu PCs): halted asynchronously - "
           "if it stopped right after a store, that store is lost (TT07)", samples.size());
    WriteReg(REG_STATUS, 1);
    for (int i = 0; i < 10; i++)                                // the halt takes a moment to show
    {
        usleep(50000);
        if (ReadReg(REG_STATUS, st) != -1 && (st & 1))
            return true;
    }
    return false;
}

// x if it is safe to halt at, else the first address after it that is
// (Cdb::safe_stop), remembered so the stop can be reported as x
uint16_t CLisa::SafeStop(uint16_t x)
{
    int     delta = 0;
    bool    ok = true;
    auto    rc = [this](uint16_t w) -> uint16_t { return CodeWord(w); };
    uint16_t s = lisa::Cdb::safe_stop(x, rc, &delta, &ok);

    if (!ok)
        Printf("warning: a stop at 0x%04x directly follows a store and cannot be moved past it (TT07: the store would be lost)", x);
    if (s != x)
        m_Shift[s] = std::make_pair(x, delta);
    return s;
}

// The core's registers as at the place meant: at a stop moved past a store
// the PC is the address meant and SP what it was there.
bool CLisa::LogicalCore(uint32_t& pc, uint32_t& sp, uint32_t& ra, uint32_t& ix, uint32_t& acc, bool *pShifted)
{
    if (pShifted)
        *pShifted = false;
    if (!ReadCore(pc, sp, ra, ix, acc))
        return false;
    auto it = m_Shift.find((uint16_t) pc);
    if (it != m_Shift.end())
    {
        pc = it->second.first;
        sp = (sp - it->second.second) & 0x7fff;
        if (pShifted)
            *pShifted = true;
    }
    return true;
}

// One instruction: breakpoints on every address it can continue at, run,
// wait for the halt (the step bit of this silicon pulses too briefly to
// act).  A store is run together with what follows it, up to an
// instruction that is safe to halt at.  Returns the new PC, -1 if it could
// not.
int CLisa::StepInsn(void)
{
    uint32_t    pc, ra, ix, saved[4];

    if (ReadReg(REG_PC, pc) == -1 || ReadReg(REG_RA, ra) == -1 || ReadReg(REG_IX, ix) == -1)
        return -1;
    uint16_t op = CodeWord(pc);
    std::vector<uint16_t> t = NextPCs(op, pc, ra & 0x7fff, ix & 0x7fff);
    // a store runs on with what follows it; the real PC is reported (not
    // remembered in m_Shift: that is for stops meant at a line)
    bool past = false;
    if (lisa::Cdb::is_store(op))
    {
        auto rc = [this](uint16_t w) -> uint16_t { return CodeWord(w); };
        for (uint16_t& a : t)
        {
            int  delta;
            bool ok;
            uint16_t s = lisa::Cdb::safe_stop(a, rc, &delta, &ok);
            if (ok && s != a)
            {
                a = s;
                past = true;
            }
        }
    }
    t.erase(std::remove(t.begin(), t.end(), (uint16_t) pc), t.end());
    std::sort(t.begin(), t.end());
    t.erase(std::unique(t.begin(), t.end()), t.end());
    if (t.empty())
    {
        Printf("cannot step at 0x%04x: the instruction only leads back to itself", pc);
        return -1;
    }
    for (int i = 0; i < 4; i++)
        ReadReg(8 + i, saved[i]);
    for (int i = 0; i < 4; i++)
        WriteReg(8 + i, i < (int) t.size() ? (0x8000 | t[i]) : 0);
    WriteReg(REG_STATUS, 2);
    bool halted = WaitHalt(2000);
    if (!halted)
        WriteReg(REG_STATUS, 1);
    for (int i = 0; i < 4; i++)
        WriteReg(8 + i, saved[i]);
    if (!halted)
    {
        Printf("the step did not complete within 2 s (halted)");
        return -1;
    }
    if (ReadReg(REG_PC, pc) == -1)
        return -1;
    m_Shift.erase((uint16_t) pc);           // here by a step: the PC is what it says
    if (past)
        Printf("(a store: ran on to 0x%04x - a halt right after a store loses it on TT07)", pc);
    return (int) pc;
}

// Run until one of the stops (at most four) is reached, the user's
// breakpoints filling the remaining registers; they are all put back
// afterwards.  A stop at the current PC needs one instruction first - a
// breakpoint there halts the core before it moves.  Returns the PC at the
// halt, -1 on a timeout (the core is halted then too).
int CLisa::RunToStops(const std::vector<uint16_t>& wanted, int timeoutMs)
{
    uint32_t    pc, saved[4];

    // never a breakpoint right after a store (the TT07 hazard): the stops
    // that would be are moved on, and reported as the stop meant
    std::vector<uint16_t> stops;
    for (uint16_t s : wanted)
        stops.push_back(SafeStop(s));
    auto logical = [this](uint32_t p) -> int {
        auto it = m_Shift.find((uint16_t) p);
        return it != m_Shift.end() ? it->second.first : (int) p;
    };

    if (ReadReg(REG_PC, pc) == -1)
        return -1;
    for (int i = 0; i < 4; i++)
        ReadReg(8 + i, saved[i]);
    bool at_user_bp = false;
    for (int i = 0; i < 4; i++)
        if ((saved[i] & 0x8000) && (saved[i] & 0x7fff) == pc)
            at_user_bp = true;
    if (at_user_bp || std::find(stops.begin(), stops.end(), (uint16_t) pc) != stops.end())
    {
        if (StepInsn() < 0 || ReadReg(REG_PC, pc) == -1)
            return -1;
        if (std::find(stops.begin(), stops.end(), (uint16_t) pc) != stops.end())
            return logical(pc);
    }
    std::vector<uint16_t> regs = stops;
    for (int i = 0; i < 4 && regs.size() < 4; i++)
        if ((saved[i] & 0x8000) && (saved[i] & 0x7fff) != pc &&
            std::find(regs.begin(), regs.end(), (uint16_t) (saved[i] & 0x7fff)) == regs.end())
            regs.push_back(saved[i] & 0x7fff);
    for (int i = 0; i < 4; i++)
        WriteReg(8 + i, i < (int) regs.size() ? (0x8000 | regs[i]) : 0);
    WriteReg(REG_STATUS, 2);
    m_Running = 1;
    bool halted = RunGranted(timeoutMs);
    if (!halted)
        WriteReg(REG_STATUS, 1);
    m_Running = 0;
    for (int i = 0; i < 4; i++)
        WriteReg(8 + i, saved[i]);
    if (!halted)
        return -1;
    return ReadReg(REG_PC, pc) == -1 ? -1 : logical(pc);
}


/*
==============================================================================
The sources
==============================================================================
*/

lisa_src_t * CLisa::SourceForFile(const std::string& file)
{
    const char  *base = basename_of(file.c_str());

    for (lisa_src_t *pSrc = m_pSrcs; pSrc; pSrc = pSrc->pNext)
        if (pSrc->cdb_file[0] && strcmp(basename_of(pSrc->cdb_file), base) == 0)
            return pSrc;
    return NULL;
}

// the line of this source that pc is on, 0 if none
int CLisa::PCLineOf(lisa_src_t *pSrc, uint32_t pc)
{
    const lisa::CdbLine *l = m_Cdb.loaded() ? m_Cdb.line_at(pc) : NULL;

    if (l && strcmp(basename_of(l->file.c_str()), basename_of(pSrc->cdb_file)) == 0)
        return l->line;
    return 0;
}

bool CLisa::IsBreakLine(lisa_src_t *pSrc, int line)
{
    if (!m_Cdb.loaded())
        return false;
    for (int x = 0; x < 4; x++)
    {
        if (!(m_Breakpoints[x] & 0x8000))
            continue;
        const lisa::CdbLine *l = m_Cdb.line_at(m_Breakpoints[x] & 0x7fff);
        if (l && l->line == line && strcmp(basename_of(l->file.c_str()), basename_of(pSrc->cdb_file)) == 0)
            return true;
    }
    return false;
}

// file:line, a line of the active tab's file, a function (its first line
// past the prologue), or *addr / 0xaddr
bool CLisa::ResolveLocation(const char *spec, uint16_t& addr, std::string& what)
{
    what.clear();
    if (spec[0] == '*' || (spec[0] == '0' && (spec[1] == 'x' || spec[1] == 'X')))
    {
        addr = strtoul(spec[0] == '*' ? spec + 1 : spec, NULL, 16) & 0x7fff;
        const lisa::CdbLine *l = m_Cdb.loaded() ? m_Cdb.line_at(addr) : NULL;
        if (l)
            what = l->file + ":" + std::to_string(l->line);
        return true;
    }
    if (!m_Cdb.loaded())
    {
        addr = strtoul(spec, NULL, 0) & 0x7fff;
        return true;
    }
    std::string file;
    int         line = 0;
    const char *colon = strrchr(spec, ':');
    bool        digits = *spec != 0;
    for (const char *p = spec; *p; p++)
        if (!isdigit((unsigned char) *p))
            digits = false;
    if (colon)
    {
        file.assign(spec, colon - spec);
        line = atoi(colon + 1);
    }
    else if (digits)
    {
        line = atoi(spec);
        // the active tab's file, else the PC's
        CTab *pTab = m_pTui->GetActiveSrcTab();
        for (lisa_src_t *pSrc = m_pSrcs; pSrc; pSrc = pSrc->pNext)
            if (pSrc->pTab == pTab && pSrc->cdb_file[0])
                file = pSrc->cdb_file;
        if (file.empty())
        {
            uint32_t pc;
            const lisa::CdbLine *here = (m_Connected && ReadReg(REG_PC, pc) != -1) ? m_Cdb.line_at(pc) : NULL;
            if (here)
                file = here->file;
            else if (!m_Cdb.files().empty())
                file = m_Cdb.files()[0];
        }
    }
    else
    {
        const lisa::CdbFunction *f = m_Cdb.function(spec);
        if (!f || !f->has_addr)
        {
            Printf("No function '%s'", spec);
            return false;
        }
        addr = m_Cdb.first_line_addr(*f);
        if (!addr)
            addr = f->addr;
        const lisa::CdbLine *l = m_Cdb.line_at(addr);
        what = f->name + "()" + (l ? " at " + l->file + ":" + std::to_string(l->line) : "");
        return true;
    }
    std::vector<uint16_t> addrs = m_Cdb.addrs_of_line(file, line);
    if (addrs.empty())
    {
        const lisa::CdbLine *nl = m_Cdb.next_line_with_code(file, line);
        if (!nl)
        {
            Printf("No code at or after %s:%d", file.c_str(), line);
            return false;
        }
        file = nl->file;
        line = nl->line;
        addrs = m_Cdb.addrs_of_line(file, line);
    }
    addr = addrs[0];
    what = file + ":" + std::to_string(line);
    return true;
}

// Report where the core is and bring that line into its tab (opening the
// tab if the source is one of the .cdb's)
void CLisa::ShowLocation(bool activate)
{
    uint32_t    pc, sp_, ra_, ix_, acc_;

    if (!m_Connected || !LogicalCore(pc, sp_, ra_, ix_, acc_))
        return;
    const lisa::CdbLine *l = m_Cdb.loaded() ? m_Cdb.line_at(pc) : NULL;
    const lisa::CdbFunction *f = m_Cdb.loaded() ? m_Cdb.function_at(pc) : NULL;
    if (l)
    {
        std::string src = m_Cdb.source(l->file, l->line);
        size_t b = src.find_first_not_of(" \t");
        Printf("0x%04x  %s:%d in %s():  %s", pc, l->file.c_str(), l->line, f->name.c_str(),
               b == std::string::npos ? "" : src.c_str() + b);
        lisa_src_t *pSrc = SourceForFile(l->file);
        if (pSrc && pSrc->pTab)
        {
            CTab *pTab = pSrc->pTab;
            int top = pTab->SourceFirstLine();
            int count = pTab->SourceWindowLineCount();
            if (!(l->line >= top && l->line < top + count))
            {
                int first = l->line - count / 2;
                if (first < 1)
                    first = 1;
                pTab->SourceFirstLine(first);
            }
            if (activate)
                pTab->SetFocus();
        }
    }
    else if (f)
        Printf("0x%04x in %s() (no line here)", pc, f->name.c_str());
    else
        Printf("0x%04x (no source: not in a C function)", pc);
    m_pParent->DrawSourceWindow();
}

// the halted frame for print/locals: the function at PC, its entry SP from
// the prologue rule, RA saved or not, A for a byte argument
lisa::Cdb::Frame CLisa::FrameHere(const lisa::CdbFunction **ppFn, uint32_t *pPc)
{
    lisa::Cdb::Frame fr;
    uint32_t    pc, sp, ra, ix, acc;
    bool        shifted;

    if (ppFn)
        *ppFn = NULL;
    if (!LogicalCore(pc, sp, ra, ix, acc, &shifted))
        return fr;
    if (pPc)
        *pPc = pc;
    auto rd = [this](uint16_t a) -> uint8_t { uint8_t b = 0; ReadRam(a, &b, 1); return b; };
    auto rc = [this](uint16_t w) -> uint16_t { return CodeWord(w); };
    std::vector<lisa::Cdb::StackFrame> frames = m_Cdb.unwind(pc, sp, ra, rd, rc, 1);
    if (frames.empty())
        return fr;
    fr.function = frames[0].function->name;
    fr.entry_sp = frames[0].entry_sp;
    fr.ra_saved = frames[0].ra_saved;
    fr.ret_slot = m_Cdb.return_slot(*frames[0].function);
    fr.a = acc & 0xff;
    fr.a_valid = !shifted;          // A may have changed past a moved stop
    if (ppFn)
        *ppFn = frames[0].function;
    return fr;
}

/*
==============================================================================
debug <prog.ihx>: the image, the .cdb beside it, the sources in tabs
==============================================================================
*/
int CLisa::Debug(int argc, char* argv[])
{
    std::string path = argv[1];
    std::string err;
    std::vector<uint16_t> words;

    if (!lisa::load_hex_image(path, words, &err))
    {
        Printf("%s", err.c_str());
        return 1;
    }
    m_Program = words;
    m_ProgramPath = path;
    std::string base = path;
    size_t dot = base.find_last_of('.');
    if (dot != std::string::npos && dot > base.find_last_of('/') + 1)
        base = base.substr(0, dot);
    m_Cdb = lisa::Cdb();
    m_Shift.clear();
    if (!m_Cdb.load(base + ".cdb", &err))
    {
        Printf("%s: %zu words, no debug info (%s: build with sdcc --debug)", path.c_str(), words.size(), err.c_str());
        return 0;
    }
    Printf("%s: %zu words; %s: %zu functions, %zu lines", path.c_str(), words.size(),
           m_Cdb.path().c_str(), m_Cdb.functions().size(), m_Cdb.lines().size());

    // the sources, each in a tab
    for (lisa_src_t *pSrc = m_pSrcs; pSrc; pSrc = pSrc->pNext)
        pSrc->cdb_file[0] = 0;
    for (const std::string& f : m_Cdb.files())
    {
        std::string p = m_Cdb.source_path(f);
        if (p.empty())
        {
            Printf("  %s: not found beside the .cdb", f.c_str());
            continue;
        }
        char *av[2] = { (char *) "open", (char *) p.c_str() };
        if (Open(2, av) != OK)
            continue;
        for (lisa_src_t *pSrc = m_pSrcs; pSrc; pSrc = pSrc->pNext)
            if (strcmp(pSrc->name, p.c_str()) == 0)
                snprintf(pSrc->cdb_file, sizeof(pSrc->cdb_file), "%s", f.c_str());
    }

    // does the flash hold this image?  A few words at main, or at 0
    if (m_Connected)
    {
        const lisa::CdbFunction *f = m_Cdb.function("main");
        uint16_t at = f && f->has_addr ? f->addr : 0;
        int bad = 0;
        for (int i = 0; i < 4 && at + i < words.size(); i++)
        {
            uint32_t v = 0;
            SetDebugAddress(m_Setup.lisa1_base + (at + i) * 2);
            if (ReadReg(0x20, v) == -1 || (uint16_t) v != words[at + i])
                bad++;
        }
        if (bad)
            Printf("warning: the flash differs from %s at 0x%04x (program it first)", basename_of(path.c_str()), at);
        uint32_t st;
        if (ReadReg(REG_STATUS, st) != -1 && !(st & 1))
            Printf("the core is running (halt, or Ctrl-C during run, stops it at a safe place)");
        else
            ShowLocation(true);
    }
    return 0;
}

/*
==============================================================================
next / into / finish: breakpoints at the line's exits, run, go on past a
deeper recursion and past a return address (the caller is mid-statement
there), until a line start; more exits than breakpoints: step instructions
==============================================================================
*/
int CLisa::StepToLine(int mode)
{
    if (!m_Connected)
        return 1;
    if (!m_Cdb.loaded())
    {
        Printf("No debug info: debug <prog.ihx> first");
        return 1;
    }
    if (m_LisaHasUart)
    {
        Printf("LISA has the UART (the terminal): leave the terminal tab first");
        return 1;
    }
    uint32_t st;
    if (ReadReg(REG_STATUS, st) == -1 || !(st & 1))
    {
        Printf("The core is running: halt it first");
        return 1;
    }
    auto rd = [this](uint16_t a) -> uint8_t { uint8_t b = 0; ReadRam(a, &b, 1); return b; };
    auto rc = [this](uint16_t w) -> uint16_t { return CodeWord(w); };
    bool finishing = mode == 2;
    for (int round = 0; ; round++)
    {
        if (round == 50)
        {
            Printf("still not at a line start after 50 runs");
            break;
        }
        uint32_t pc, sp, ra, ix, acc;
        if (!LogicalCore(pc, sp, ra, ix, acc))
            return 1;
        std::vector<lisa::Cdb::StackFrame> frames = m_Cdb.unwind(pc, sp, ra, rd, rc, 1);
        uint16_t ret = frames.empty() ? 0 : frames[0].ret_pc;
        uint16_t entry_sp = frames.empty() ? sp : frames[0].entry_sp;
        const lisa::CdbFunction *fn = m_Cdb.function_at(pc);
        std::vector<uint16_t> stops;
        bool stepping = false;
        if (finishing)
        {
            if (!ret)
            {
                Printf("no return address known here");
                return 1;
            }
            stops.push_back(ret);
        }
        else
        {
            lisa::Cdb::Exits ex = m_Cdb.line_exits(pc, mode == 1, ret, rc);
            if (ex.unknown)
                Printf("(a path out of this line is unaccounted for: a jmp ix, or no return address)");
            stops = ex.stops;
            if (stops.size() > 4)
            {
                Printf("(%zu exits, more than the breakpoints: stepping instructions)", stops.size());
                stepping = true;
            }
        }
        int now;
        if (stepping)
        {
            // instruction by instruction until a line start (or out of the function)
            now = -1;
            for (int i = 0; i < 3000; i++)
            {
                int npc = StepInsn();
                if (npc >= 0 && m_Shift.count((uint16_t) npc))
                    npc = m_Shift[(uint16_t) npc].first;
                if (npc < 0)
                    return 1;
                if (m_Cdb.line_starting(npc) || !m_Cdb.function_at(npc) || (mode == 0 && m_Cdb.function_at(npc) != fn && std::find(stops.begin(), stops.end(), (uint16_t) npc) != stops.end()))
                {
                    now = npc;
                    break;
                }
                if (gLastWasCtrlC)
                {
                    gLastWasCtrlC = 0;
                    Printf("interrupted");
                    now = npc;
                    break;
                }
            }
            if (now < 0)
            {
                Printf("no line start within 3000 instructions");
                break;
            }
        }
        else
        {
            now = RunToStops(stops, 60000);
            if (now < 0)
            {
                Printf("no stop within 60 s (halted where it was)");
                break;
            }
        }
        bool ours = std::find(stops.begin(), stops.end(), (uint16_t) now) != stops.end();
        if (!ours && !stepping)
            break;                                                      // a breakpoint of the user's
        if (!m_Cdb.function_at(now))
        {
            Printf("(left the C code at 0x%04x)", now);
            break;
        }
        uint32_t nsp, npc_, nra_, nix_, nacc_;
        if (!LogicalCore(npc_, nsp, nra_, nix_, nacc_))
            return 1;
        if (finishing)
        {
            if (nsp < entry_sp)
                continue;                                               // a deeper frame returned here
            finishing = false;                                          // mid-statement in the caller: on to its next line
            continue;
        }
        if (m_Cdb.line_starting(now))
        {
            if (nsp < sp && m_Cdb.function_at(now) == fn)
                continue;                                               // the same line, one recursion deeper
            break;
        }
        // a return address: on to the caller's next line
    }
    ShowLocation(true);
    return 0;
}

// what the source-level views read is only meaningful on a halted core
bool CLisa::RequireHalted(void)
{
    uint32_t st;

    if (ReadReg(REG_STATUS, st) == -1)
        return false;
    if (!(st & 1))
    {
        Printf("The core is running: halt it first");
        return false;
    }
    return true;
}

int CLisa::Next(int argc, char* argv[])   { return StepToLine(0); }
int CLisa::Into(int argc, char* argv[])   { return StepToLine(1); }
int CLisa::Finish(int argc, char* argv[]) { return StepToLine(2); }

int CLisa::Where(int argc, char* argv[])
{
    if (!m_Connected || !RequireHalted())
        return 1;
    ShowLocation(true);
    return 0;
}

/*
==============================================================================
print / locals / bt
==============================================================================
*/
int CLisa::Print(int argc, char* argv[])
{
    if (!m_Connected || !RequireHalted())
        return 1;
    if (!m_Cdb.loaded())
    {
        Printf("No debug info: debug <prog.ihx> first");
        return 1;
    }
    std::string expr;
    for (int i = 1; i < argc; i++)
        expr += argv[i];
    // the data, read 16 bytes at a time around what is asked for
    uint16_t    cbase = 0;
    int         clen = 0;
    uint8_t     cbuf[16];
    auto rd = [&](uint16_t a) -> uint8_t {
        if (!(clen && a >= cbase && a < cbase + clen))
        {
            cbase = a;
            clen = ReadRam(a, cbuf, 16);
            if (clen <= 0) { clen = 0; return 0; }
        }
        return cbuf[a - cbase];
    };
    auto rdp = [](uint16_t) -> uint8_t { return 0; };    // peripheral registers: not readable through the debugger
    auto rc = [this](uint16_t w) -> uint16_t { return CodeWord(w); };
    const lisa::CdbFunction *fn;
    lisa::Cdb::Frame fr = FrameHere(&fn);
    std::string err;
    std::string s = m_Cdb.print(expr, fn ? &fr : NULL, &err, rd, rdp, rc);
    if (s.empty())
        Printf("%s", err.c_str());
    else
        Printf("%s = %s", expr.c_str(), s.c_str());
    return 0;
}

int CLisa::Locals(int argc, char* argv[])
{
    if (!m_Connected || !RequireHalted())
        return 1;
    if (!m_Cdb.loaded())
    {
        Printf("No debug info: debug <prog.ihx> first");
        return 1;
    }
    uint16_t    cbase = 0;
    int         clen = 0;
    uint8_t     cbuf[16];
    auto rd = [&](uint16_t a) -> uint8_t {
        if (!(clen && a >= cbase && a < cbase + clen))
        {
            cbase = a;
            clen = ReadRam(a, cbuf, 16);
            if (clen <= 0) { clen = 0; return 0; }
        }
        return cbuf[a - cbase];
    };
    auto rdp = [](uint16_t) -> uint8_t { return 0; };
    auto rc = [this](uint16_t w) -> uint16_t { return CodeWord(w); };
    const lisa::CdbFunction *fn;
    uint32_t pc;
    lisa::Cdb::Frame fr = FrameHere(&fn, &pc);
    if (!fn)
    {
        Printf("0x%04x is not in a C function", pc);
        return 1;
    }
    Printf("%s(), entry SP=0x%04x%s:", fn->name.c_str(), fr.entry_sp, fr.ra_saved ? ", RA saved" : "");
    for (int pass = 0; pass < 2; pass++)
    {
        for (const lisa::CdbSymbol *s : m_Cdb.locals(fn->name))
        {
            bool arg = (s->space == 'B' && s->offset >= 0) || s->space == 'R';
            if (arg != (pass == 0))
                continue;
            if (s->name.rfind("sloc", 0) == 0)
                continue;
            lisa::Cdb::Value v;
            if (m_Cdb.variable(s->name, &fr, v))
                Printf("  %s%s = %s   %s%s", pass == 0 ? "arg " : "", s->name.c_str(),
                       m_Cdb.format_value(v, rd, rdp, rc).c_str(), s->type.describe().c_str(), s->space == 'R' ? " in A" : "");
            else
                Printf("  %s%s = <no location>   %s", pass == 0 ? "arg " : "", s->name.c_str(), s->type.describe().c_str());
        }
    }
    return 0;
}

int CLisa::Backtrace(int argc, char* argv[])
{
    if (!m_Connected || !RequireHalted())
        return 1;
    if (!m_Cdb.loaded())
    {
        Printf("No debug info: debug <prog.ihx> first");
        return 1;
    }
    uint32_t pc, sp, ra, ix, acc;
    bool     shifted;
    if (!LogicalCore(pc, sp, ra, ix, acc, &shifted))
        return 1;
    uint16_t    cbase = 0;
    int         clen = 0;
    uint8_t     cbuf[16];
    auto rd = [&](uint16_t a) -> uint8_t {
        if (!(clen && a >= cbase && a < cbase + clen))
        {
            cbase = a;
            clen = ReadRam(a, cbuf, 16);
            if (clen <= 0) { clen = 0; return 0; }
        }
        return cbuf[a - cbase];
    };
    auto rdp = [](uint16_t) -> uint8_t { return 0; };
    auto rc = [this](uint16_t w) -> uint16_t { return CodeWord(w); };
    std::vector<lisa::Cdb::StackFrame> frames = m_Cdb.unwind(pc, sp, ra, rd, rc);
    if (frames.empty())
    {
        Printf("#0  0x%04x  ?? (not in a C function)", pc);
        return 0;
    }
    for (size_t i = 0; i < frames.size(); i++)
    {
        const lisa::Cdb::StackFrame& fr = frames[i];
        lisa::Cdb::Frame cf;
        cf.function = fr.function->name;
        cf.entry_sp = fr.entry_sp;
        cf.ra_saved = fr.ra_saved;
        cf.ret_slot = m_Cdb.return_slot(*fr.function);
        cf.a = acc & 0xff;
        cf.a_valid = i == 0 && !shifted;
        std::string params;
        for (const lisa::CdbSymbol *s : m_Cdb.locals(fr.function->name))
        {
            if (!((s->space == 'B' && s->offset >= 0) || (s->space == 'R' && i == 0)))
                continue;
            lisa::Cdb::Value v;
            if (!params.empty())
                params += ", ";
            params += s->name + "=" + (m_Cdb.variable(s->name, &cf, v) ? m_Cdb.format_value(v, rd, rdp, rc) : "?");
        }
        const lisa::CdbLine *l = m_Cdb.line_at(i == 0 ? fr.pc : fr.pc - 1);
        Printf("#%-2zu 0x%04x  %s(%s)%s%s%s", i, fr.pc, fr.function->name.c_str(), params.c_str(),
               l ? " at " : "", l ? l->file.c_str() : "", l ? (":" + std::to_string(l->line)).c_str() : "");
    }
    return 0;
}

// vim: sw=4 ts=4 et
