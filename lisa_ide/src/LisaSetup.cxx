// LisaSetup.cxx - the SETUP tab: a form over the debugger's configuration
// registers (debug_regs.v) - the chip select and base address of program
// fetch (LISA1), the data cache (LISA2), the TTLC and the debugger's own
// flash port; per chip select flash/RAM, SPI/QSPI, 24/16-bit addresses and
// dummy cycles; the SPI mode, clock and CE delay; the data cache; the pin
// muxes.  OK writes and reads back the registers and keeps the settings
// (applied on every connect, saved in .tui_prefs); Cancel drops the edits.
#include "Lisa.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

extern int gLastWasCtrlC;

/*
==============================================================================
The settings and their registers
==============================================================================
*/
LisaSetupCfg LisaSetupCfg::Defaults(void)
{
    // what connect wrote before the tab existed (the Commander's setup()),
    // and the TTLC where the Commander puts it
    LisaSetupCfg c;
    c.lisa1_cs = 0;  c.lisa1_base = 0;
    c.lisa2_cs = 1;  c.lisa2_base = 0;
    c.ttlc_cs  = 0;  c.ttlc_base  = 0x10000;
    c.dbg_cs   = 0;
    c.is_flash[0] = 1; c.quad[0] = 0; c.addr16[0] = 0; c.dummy[0] = 0;
    c.is_flash[1] = 0; c.quad[1] = 0; c.addr16[1] = 0; c.dummy[1] = 0;
    c.spi_mode = 1;  c.sclk_div = 3;  c.ce_delay = 13;
    c.cache_on = 0;  c.cache_map = 3; c.shift_div = 0;
    c.io_mux   = 0;  c.out_mux = 0x5455;
    return c;
}

std::vector<std::pair<uint8_t, uint16_t> > LisaSetupCfg::Registers(void) const
{
    std::vector<std::pair<uint8_t, uint16_t> > r;
    r.push_back({0x12, (uint16_t) (lisa1_base >> 8)});
    r.push_back({0x13, (uint16_t) (lisa2_base >> 8)});
    r.push_back({0x1f, (uint16_t) (ttlc_base >> 8)});
    r.push_back({0x14, (uint16_t) (1 << lisa1_cs)});
    r.push_back({0x15, (uint16_t) (((1 << ttlc_cs) << 2) | (1 << lisa2_cs))});
    r.push_back({0x16, (uint16_t) (1 << dbg_cs)});
    r.push_back({0x17, (uint16_t) ((addr16[1] << 5) | (addr16[0] << 4) | (is_flash[1] << 3) | (is_flash[0] << 2) |
                                   (quad[1] << 1) | quad[0])});
    r.push_back({0x18, (uint16_t) (((dummy[1] & 15) << 4) | (dummy[0] & 15))});
    r.push_back({0x1e, (uint16_t) (((spi_mode & 3) << 11) | ((ce_delay & 0x7f) << 4) | (sclk_div & 15))});
    r.push_back({0x1c, (uint16_t) (((shift_div & 3) << 8) | (io_mux & 0xff))});
    r.push_back({0x1b, (uint16_t) out_mux});
    // the caches last: invalidate the instruction cache (the fetch base may
    // have moved), and the data cache when it is turned on
    r.push_back({0x1d, (uint16_t) ((cache_on ? 0x10 : 0x04) | 0x20 | (cache_map & 3))});
    return r;
}

static int cs_of(uint16_t mask) { return (mask & 2) && !(mask & 1) ? 1 : 0; }

bool LisaSetupCfg::FromRegisters(const std::map<uint8_t, uint16_t>& r)
{
    auto g = [&](uint8_t a) -> uint16_t { auto it = r.find(a); return it == r.end() ? 0 : it->second; };
    lisa1_base = (uint32_t) g(0x12) << 8;
    lisa2_base = (uint32_t) g(0x13) << 8;
    ttlc_base  = (uint32_t) g(0x1f) << 8;
    lisa1_cs = cs_of(g(0x14));
    lisa2_cs = cs_of(g(0x15) & 3);
    ttlc_cs  = cs_of((g(0x15) >> 2) & 3);
    dbg_cs   = cs_of(g(0x16));
    uint16_t m = g(0x17);
    for (int i = 0; i < 2; i++)
    {
        quad[i]     = (m >> i) & 1;
        is_flash[i] = (m >> (2 + i)) & 1;
        addr16[i]   = (m >> (4 + i)) & 1;
        dummy[i]    = (g(0x18) >> (4 * i)) & 15;
    }
    uint16_t sp = g(0x1e);
    spi_mode = (sp >> 11) & 3;
    ce_delay = (sp >> 4) & 0x7f;
    sclk_div = sp & 15;
    shift_div = (g(0x1c) >> 8) & 3;
    io_mux    = g(0x1c) & 0xff;
    out_mux   = g(0x1b);
    cache_on  = !(g(0x1d) & 4);
    cache_map = g(0x1d) & 3;
    return true;
}

std::string LisaSetupCfg::Validate(void) const
{
    if (lisa1_base & 0xff) return "the program fetch base must be a multiple of 0x100";
    if (lisa2_base & 0xff) return "the data cache base must be a multiple of 0x100";
    if (ttlc_base & 0xff)  return "the TTLC base must be a multiple of 0x100";
    if (lisa1_base > 0xffff00 || lisa2_base > 0xffff00 || ttlc_base > 0xffff00) return "base addresses are 24 bits";
    if (!is_flash[lisa1_cs]) return "program fetch needs a flash on its chip select";
    if (cache_on && is_flash[lisa2_cs]) return "the data cache needs a RAM on its chip select (Device: RAM)";
    if (cache_on && lisa2_cs == lisa1_cs) return "the data cache and program fetch share a chip select";
    return "";
}

std::string LisaSetupCfg::Serialize(void) const
{
    char b[256];
    snprintf(b, sizeof b, "1 %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
             lisa1_cs, lisa1_base, lisa2_cs, lisa2_base, ttlc_cs, ttlc_base, dbg_cs,
             is_flash[0], quad[0], addr16[0], dummy[0], is_flash[1], quad[1], addr16[1], dummy[1],
             spi_mode, sclk_div, ce_delay, cache_on, cache_map, shift_div, io_mux, out_mux);
    return b;
}

bool LisaSetupCfg::Parse(const char *s)
{
    unsigned v[24];
    if (sscanf(s, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
               &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10], &v[11],
               &v[12], &v[13], &v[14], &v[15], &v[16], &v[17], &v[18], &v[19], &v[20], &v[21], &v[22], &v[23]) != 24
        || v[0] != 1)
        return false;
    lisa1_cs = v[1] & 1; lisa1_base = v[2]; lisa2_cs = v[3] & 1; lisa2_base = v[4];
    ttlc_cs = v[5] & 1; ttlc_base = v[6]; dbg_cs = v[7] & 1;
    is_flash[0] = v[8] & 1; quad[0] = v[9] & 1; addr16[0] = v[10] & 1; dummy[0] = v[11] & 15;
    is_flash[1] = v[12] & 1; quad[1] = v[13] & 1; addr16[1] = v[14] & 1; dummy[1] = v[15] & 15;
    spi_mode = v[16] & 3; sclk_div = v[17] & 15; ce_delay = v[18] & 0x7f;
    cache_on = v[19] & 1; cache_map = v[20] & 3; shift_div = v[21] & 3; io_mux = v[22] & 0xff; out_mux = v[23] & 0xffff;
    return true;
}

/*
==============================================================================
On the chip
==============================================================================
*/
static const uint8_t gSetupRegs[] = { 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };

bool CLisa::ReadSetup(LisaSetupCfg& cfg)
{
    std::map<uint8_t, uint16_t> r;
    uint32_t v;

    for (uint8_t a : gSetupRegs)
    {
        if (ReadReg(a, v) == -1)
            return false;
        r[a] = (uint16_t) v;
    }
    return cfg.FromRegisters(r);
}

// the debugger's flash port address (registers 0x11:0x10)
void CLisa::SetDebugAddress(uint32_t byteAddr)
{
    WriteReg(0x11, (byteAddr >> 16) & 0xff);
    WriteReg(0x10, byteAddr & 0xffff);
}

// Write the registers, read each back (the cache register's invalidate
// bits clear themselves: only its low three bits are compared)
int CLisa::ApplySetup(const LisaSetupCfg& cfg, std::string *pReport)
{
    std::string bad;
    uint32_t    v;

    for (const auto& rv : cfg.Registers())
        WriteReg(rv.first, rv.second);
    SetDebugAddress(cfg.lisa1_base);
    for (const auto& rv : cfg.Registers())
    {
        uint16_t mask = rv.first == 0x1d ? 0x07 : 0xffff;
        if (ReadReg(rv.first, v) == -1 || ((uint16_t) v & mask) != (rv.second & mask))
        {
            char b[64];
            snprintf(b, sizeof b, "%s0x%02x reads 0x%04x (wrote 0x%04x)", bad.empty() ? "" : ", ", rv.first, v, rv.second);
            bad += b;
        }
    }
    m_Setup = cfg;
    if (pReport)
        *pReport = bad;
    return bad.empty() ? 0 : -1;
}

// connect's setup: the saved settings (the defaults until the tab is used)
int CLisa::SetupDebugger(uint32_t flashBase)
{
    LisaSetupCfg cfg = m_Setup;
    std::string  bad;

    if (flashBase)
        cfg.lisa1_base = flashBase;
    if (ApplySetup(cfg, &bad) != 0)
    {
        Printf("debugger setup: %s", bad.c_str());
        return -1;
    }
    Printf("debugger registers set up (program fetch CS%d at 0x%06x, data cache %s; `setup` to change)",
           cfg.lisa1_cs, cfg.lisa1_base, cfg.cache_on ? "on" : "off");
    if (cfg.cache_on)
        Printf("note: with the data cache on, data lives in the RAM on CS%d - on the demo board the RP2040's "
               "SPI RAM emulation (the LISA Commander starts it); programs for the 128-byte RAM fail without it",
               cfg.lisa2_cs);
    return 0;
}

// setup             the SETUP tab
// setup apply       write the saved settings
// setup defaults    write and keep the defaults
int CLisa::Setup(int argc, char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "defaults") == 0)
    {
        m_Setup = LisaSetupCfg::Defaults();
        m_pParent->WriteUIPreferences();
        if (!m_Connected)
        {
            Printf("defaults kept; written on connect");
            return 0;
        }
        return SetupDebugger(0);
    }
    if (argc > 1 && strcmp(argv[1], "apply") == 0)
    {
        if (!m_Connected)
            return 1;
        return SetupDebugger(0);
    }
    if (argc > 1)
    {
        Printf("Usage: setup [apply | defaults]   (no argument: the SETUP tab)");
        return 1;
    }
    OpenSetupTab();
    return 0;
}

/*
==============================================================================
The form
==============================================================================
*/
enum { F_CHOICE, F_HEX, F_DEC, F_BUTTON };
enum { B_OK = 1, B_CANCEL, B_READ, B_DEFAULTS };

struct SetupField
{
    int             row, col;
    int             kind;
    const char *    label;          // drawn before the field (may be NULL)
    int             labelCol;
    int             offset;         // of an int (or uint32_t for F_HEX with digits 6) in LisaSetupCfg
    const char *const *choices;
    int             nchoices;
    int             digits;         // F_HEX / F_DEC
    uint32_t        max;
    int             button;
};

static const char *const gCs[]     = { "CS0", "CS1" };
static const char *const gDevice[] = { "RAM", "flash" };
static const char *const gBus[]    = { "SPI", "QSPI" };
static const char *const gAddr[]   = { "24-bit", "16-bit" };
static const char *const gOnOff[]  = { "off", "on" };
static const char *const gShift[]  = { "/4", "/8", "/16", "/32" };
static const char *const gMode[]   = { "0", "1", "2", "3" };

#define OFF(m) ((int) offsetof(LisaSetupCfg, m))

static const SetupField gFields[] =
{
    // row col  kind      label                  lcol  field                 choices   n  dig max
    {  5, 24, F_CHOICE, "Program fetch (LISA1)",  2, OFF(lisa1_cs),        gCs,     2, 0, 0, 0},
    {  5, 42, F_HEX,    NULL,                     0, OFF(lisa1_base),      NULL,    0, 6, 0xffff00, 0},
    {  6, 24, F_CHOICE, "Data cache (LISA2)",     2, OFF(lisa2_cs),        gCs,     2, 0, 0, 0},
    {  6, 42, F_HEX,    NULL,                     0, OFF(lisa2_base),      NULL,    0, 6, 0xffff00, 0},
    {  6, 62, F_CHOICE, "cache",                 55, OFF(cache_on),        gOnOff,  2, 0, 0, 0},
    {  6, 80, F_DEC,    "map",                   75, OFF(cache_map),       NULL,    0, 1, 3, 0},
    {  7, 24, F_CHOICE, "TTLC",                   2, OFF(ttlc_cs),         gCs,     2, 0, 0, 0},
    {  7, 42, F_HEX,    NULL,                     0, OFF(ttlc_base),       NULL,    0, 6, 0xffff00, 0},
    {  7, 62, F_CHOICE, "shift",                 55, OFF(shift_div),       gShift,  4, 0, 0, 0},
    {  8, 24, F_CHOICE, "Debugger flash port",    2, OFF(dbg_cs),          gCs,     2, 0, 0, 0},
    { 12, 24, F_CHOICE, "Device",                 2, OFF(is_flash[0]),     gDevice, 2, 0, 0, 0},
    { 12, 38, F_CHOICE, NULL,                     0, OFF(is_flash[1]),     gDevice, 2, 0, 0, 0},
    { 13, 24, F_CHOICE, "Bus",                    2, OFF(quad[0]),         gBus,    2, 0, 0, 0},
    { 13, 38, F_CHOICE, NULL,                     0, OFF(quad[1]),         gBus,    2, 0, 0, 0},
    { 14, 24, F_CHOICE, "Address",                2, OFF(addr16[0]),       gAddr,   2, 0, 0, 0},
    { 14, 38, F_CHOICE, NULL,                     0, OFF(addr16[1]),       gAddr,   2, 0, 0, 0},
    { 15, 24, F_DEC,    "Dummy read cycles",      2, OFF(dummy[0]),        NULL,    0, 2, 15, 0},
    { 15, 38, F_DEC,    NULL,                     0, OFF(dummy[1]),        NULL,    0, 2, 15, 0},
    { 18, 24, F_CHOICE, "SPI mode",               2, OFF(spi_mode),        gMode,   4, 0, 0, 0},
    { 18, 50, F_DEC,    "SCLK div",              41, OFF(sclk_div),        NULL,    0, 2, 15, 0},
    { 18, 69, F_DEC,    "CE delay",              60, OFF(ce_delay),        NULL,    0, 3, 127, 0},
    { 19, 24, F_HEX,    "uio mux (0x1c)",         2, OFF(io_mux),          NULL,    0, 2, 0xff, 0},
    { 19, 50, F_HEX,    "uo_out",                41, OFF(out_mux),         NULL,    0, 4, 0xffff, 0},
    { 21,  4, F_BUTTON, NULL,                     0, 0,                    NULL,    0, 0, 0, B_OK},
    { 21, 16, F_BUTTON, NULL,                     0, 0,                    NULL,    0, 0, 0, B_CANCEL},
    { 21, 30, F_BUTTON, NULL,                     0, 0,                    NULL,    0, 0, 0, B_READ},
    { 21, 50, F_BUTTON, NULL,                     0, 0,                    NULL,    0, 0, 0, B_DEFAULTS},
};
static const int gNFields = sizeof(gFields) / sizeof(gFields[0]);

// the field's value (base addresses are uint32_t, everything else int)
static uint32_t field_get(const LisaSetupCfg& c, const SetupField& f)
{
    const char *p = (const char *) &c + f.offset;
    if (f.kind == F_HEX && f.digits == 6)
        return *(const uint32_t *) p;
    return (uint32_t) *(const int *) p;
}

static void field_set(LisaSetupCfg& c, const SetupField& f, uint32_t v)
{
    char *p = (char *) &c + f.offset;
    if (f.kind == F_HEX && f.digits == 6)
        *(uint32_t *) p = v;
    else
        *(int *) p = (int) v;
}

bool CLisa::IsSetupTab(void)
{
    return m_pSetupSrc && m_pSetupSrc->pTab && m_pTui->GetActiveSrcTab() == m_pSetupSrc->pTab;
}

void CLisa::OpenSetupTab(void)
{
    if (m_pSetupSrc && m_pSetupSrc->pTab)
    {
        m_pSetupSrc->pTab->SetFocus();
        m_pParent->FocusTabs();
        m_pParent->DrawSourceWindow();
        return;
    }
    // what is on the chip, else what will be written on connect
    m_SetupEdit = m_Setup;
    m_SetupStatus.clear();
    if (m_Connected && !m_LisaHasUart)
    {
        if (ReadSetup(m_SetupEdit))
            m_SetupStatus = "read from the chip";
        else
        {
            m_SetupEdit = m_Setup;
            m_SetupStatus = "could not read the chip: showing the saved settings";
        }
    }
    else
        m_SetupStatus = "not connected: OK keeps the settings for the next connect";

    lisa_src_t *pSrc = (lisa_src_t *) calloc(1, sizeof(lisa_src_t));
    if (pSrc == NULL)
        return;
    pSrc->topLine = 1;
    pSrc->type    = LISA_SRC_TYPE_SETUP;
    strcpy(pSrc->name, "SETUP");
    CTab *pTab = m_pParent->CreateNewTab("SETUP");
    if (pTab == NULL)
    {
        free(pSrc);
        return;
    }
    pSrc->pNext = m_pSrcs;
    m_pSrcs = pSrc;
    pTab->AttachTuiSource(this, (void *) pSrc);
    pTab->SourceFirstLine(1);
    pSrc->pTab = pTab;
    m_pSetupSrc = pSrc;
    m_SetupField = 0;
    m_SetupFresh = true;
    pTab->SetFocus();
    m_pParent->FocusTabs();
    m_pParent->DrawSourceWindow();
}

void CLisa::CloseSetupTab(void)
{
    lisa_src_t *pSrc = m_pSetupSrc;

    if (!pSrc || !pSrc->pTab)
        return;
    m_pSetupSrc = NULL;
    CloseTab(pSrc->pTab);           // frees pSrc
    m_pParent->FocusCommand();
}

void CLisa::DrawSetupWindow(WINDOW *pWnd)
{
    const LisaSetupCfg& c = m_SetupEdit;
    static const char *buttons[] = { "", "   OK   ", " Cancel ", " Read from chip ", " Defaults " };

    werase(pWnd);
    wattron(pWnd, COLOR_PAIR(SYNTAX_PAIR_NORMAL));
    wattron(pWnd, A_BOLD);
    mvwprintw(pWnd, 1, 2, "LISA debugger setup");
    wattroff(pWnd, A_BOLD);
    mvwprintw(pWnd, 2, 2, "Up/Down/Tab: field  Left/Right/Space: change  digits/Backspace: edit  Enter: button  Esc: Cancel");
    mvwprintw(pWnd, 4, 24, "chip select");
    mvwprintw(pWnd, 4, 42, "base address");
    mvwprintw(pWnd, 10, 2, "Chip selects");
    mvwprintw(pWnd, 11, 24, "CS0");
    mvwprintw(pWnd, 11, 38, "CS1");
    mvwprintw(pWnd, 17, 2, "SPI and pins");
    for (int i = 0; i < gNFields; i++)
    {
        const SetupField& f = gFields[i];
        char buf[64];
        if (f.label)
            mvwprintw(pWnd, f.row, f.labelCol, "%s", f.label);
        switch (f.kind)
        {
        case F_CHOICE:
            snprintf(buf, sizeof buf, "< %-6s >", f.choices[field_get(c, f) % f.nchoices]);
            break;
        case F_HEX:
            snprintf(buf, sizeof buf, "[0x%0*X]", f.digits, field_get(c, f));
            break;
        case F_DEC:
            snprintf(buf, sizeof buf, "[%*u]", f.digits, field_get(c, f));
            break;
        default:
            snprintf(buf, sizeof buf, "[%s]", buttons[f.button]);
            break;
        }
        if (i == m_SetupField)
            wattron(pWnd, A_REVERSE);
        mvwprintw(pWnd, f.row, f.col, "%s", buf);
        if (i == m_SetupField)
            wattroff(pWnd, A_REVERSE);
    }
    // what the settings write, and anything wrong with them
    std::string regs[2];
    int n = 0;
    for (const auto& rv : c.Registers())
    {
        char b[16];
        snprintf(b, sizeof b, "%02x=%04x  ", rv.first, rv.second);
        regs[n++ < 6 ? 0 : 1] += b;
    }
    mvwprintw(pWnd, 23, 2, "registers  %s", regs[0].c_str());
    mvwprintw(pWnd, 24, 2, "           %s", regs[1].c_str());
    std::string err = c.Validate();
    if (!err.empty())
    {
        wattron(pWnd, COLOR_PAIR(SYNTAX_PAIR_COMMENT) | A_BOLD);
        mvwprintw(pWnd, 26, 2, "! %s", err.c_str());
        wattroff(pWnd, COLOR_PAIR(SYNTAX_PAIR_COMMENT) | A_BOLD);
    }
    else if (!m_SetupStatus.empty())
        mvwprintw(pWnd, 26, 2, "%s", m_SetupStatus.c_str());
    wrefresh(pWnd);
}

int CLisa::SetupKey(int key)
{
    if (!IsSetupTab())
        return 0;
    const SetupField& f = gFields[m_SetupField];
    LisaSetupCfg& c = m_SetupEdit;
    bool redraw = true;

    switch (key)
    {
    case 27:                                                    // Esc: Cancel
        Printf("setup cancelled");
        CloseSetupTab();
        return 1;
    case KEY_UP:
    case KEY_BTAB:
        m_SetupField = (m_SetupField + gNFields - 1) % gNFields;
        m_SetupFresh = true;
        break;
    case KEY_DOWN:
    case 9:                                                     // Tab
        m_SetupField = (m_SetupField + 1) % gNFields;
        m_SetupFresh = true;
        break;
    case KEY_LEFT:
    case KEY_RIGHT:
    case ' ':
    {
        int d = key == KEY_LEFT ? -1 : 1;
        if (f.kind == F_CHOICE)
            field_set(c, f, (field_get(c, f) + f.nchoices + d) % f.nchoices);
        else if (f.kind == F_HEX || f.kind == F_DEC)
        {
            uint32_t step = (f.kind == F_HEX && f.digits == 6) ? 0x100 : 1;
            int64_t  v = (int64_t) field_get(c, f) + d * (int64_t) step;
            if (v < 0) v = 0;
            if (v > (int64_t) f.max) v = f.max;
            field_set(c, f, (uint32_t) v);
            m_SetupFresh = true;
        }
        else if (key == ' ')
            return SetupKey('\n');
        break;
    }
    case KEY_BACKSPACE:
    case 127:
    case 8:
        if (f.kind == F_HEX)
            field_set(c, f, field_get(c, f) >> 4);
        else if (f.kind == F_DEC)
            field_set(c, f, field_get(c, f) / 10);
        m_SetupFresh = false;
        break;
    case '\n':
    case '\r':
    case KEY_ENTER:
        if (f.kind != F_BUTTON)
        {
            m_SetupField = (m_SetupField + 1) % gNFields;   // Enter in a field: on to the next
            m_SetupFresh = true;
            break;
        }
        if (f.button == B_CANCEL)
        {
            Printf("setup cancelled");
            CloseSetupTab();
            return 1;
        }
        if (f.button == B_DEFAULTS)
        {
            c = LisaSetupCfg::Defaults();
            m_SetupStatus = "defaults (not written yet: OK writes them)";
            break;
        }
        if (f.button == B_READ)
        {
            if (!m_Connected || m_LisaHasUart)
                m_SetupStatus = "not connected";
            else if (ReadSetup(c))
                m_SetupStatus = "read from the chip";
            else
                m_SetupStatus = "could not read the chip";
            break;
        }
        // OK
        {
            std::string err = c.Validate();
            if (!err.empty())
            {
                m_SetupStatus = err;
                break;
            }
            if (!m_Connected)
            {
                m_Setup = c;
                Printf("setup kept: written on connect");
            }
            else
            {
                std::string bad;
                if (ApplySetup(c, &bad) == 0)
                    Printf("setup written and verified: program fetch CS%d at 0x%06x, data cache %s on CS%d, TTLC CS%d at 0x%06x",
                           c.lisa1_cs, c.lisa1_base, c.cache_on ? "on" : "off", c.lisa2_cs, c.ttlc_cs, c.ttlc_base);
                else
                    Printf("setup written, but %s", bad.c_str());
                if (c.cache_on)
                    Printf("note: the data cache needs the RAM on CS%d (the RP2040's SPI RAM emulation on the demo board)", c.lisa2_cs);
            }
            m_pParent->WriteUIPreferences();                // keep it
            CloseSetupTab();
            return 1;
        }
    default:
    {
        // a digit edits a number: the first one after arriving replaces it
        int d = -1;
        if (key >= '0' && key <= '9') d = key - '0';
        else if (f.kind == F_HEX && key >= 'a' && key <= 'f') d = key - 'a' + 10;
        else if (f.kind == F_HEX && key >= 'A' && key <= 'F') d = key - 'A' + 10;
        if (d < 0 || (f.kind != F_HEX && f.kind != F_DEC))
            return key < 0x20 || key > 0x7e ? 0 : 1;            // printable keys stay in the form
        uint32_t base = f.kind == F_HEX ? 16 : 10;
        uint64_t v = m_SetupFresh ? 0 : field_get(c, f);
        v = v * base + d;
        if (v > f.max)
            v = d;                                              // overflow: start again with this digit
        field_set(c, f, (uint32_t) v);
        m_SetupFresh = false;
        break;
    }
    }
    if (redraw)
        m_pParent->DrawSourceWindow();
    return 1;
}

/*
==============================================================================
Kept in .tui_prefs
==============================================================================
*/
void CLisa::SaveWatchItems(FILE *fd)
{
    fprintf(fd, "LISA_SETUP=%s\n", m_Setup.Serialize().c_str());
}

void CLisa::RestoreOtherPref(char *pPref, char *pStr)
{
    if (strcmp(pPref, "LISA_SETUP") == 0)
    {
        LisaSetupCfg c = LisaSetupCfg::Defaults();
        if (c.Parse(pStr))
            m_Setup = c;
    }
}

// vim: sw=4 ts=4 et
