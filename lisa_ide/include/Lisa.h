#pragma once

#include <string>
#include <vector>
#include <map>
#include "Tui.h"
#include "simevent.h"
#include "serial.h"     // assuming this is your ser_* interface
#include "cdb.h"        // sdcc --debug's .cdb (../lisa_cdb, shared with lisa_sim)

#define REG_STATUS          0
#define REG_ACC             1
#define REG_PC              2
#define REG_SP              3
#define REG_RA              4
#define REG_IX              5
#define REG_FLAGS           1
#define READ_RETRY_COUNT    5

#define LISA_SRC_TYPE_ASM         0
#define LISA_SRC_TYPE_LIST        1
#define LISA_SRC_TYPE_REL         2
#define LISA_SRC_TYPE_C           3
#define LISA_SRC_TYPE_OBJ         4
#define LISA_SRC_TYPE_TERM        5
#define LISA_SRC_TYPE_CONFIG      6
#define LISA_SRC_TYPE_OTHER       7
#define LISA_SRC_TYPE_SETUP       8

#define LISA_MAX_PATHS            16

class CLisa;
typedef int (CLisa::*CLisaFunc_t)(int argc, char* argv[]);

// The debugger's configuration registers (debug_regs.v), as the SETUP tab
// edits them: which chip select each client uses, where in it, and how
// each chip select's device is driven.  Base addresses are bytes (the
// registers hold them >> 8).
struct LisaSetupCfg
{
    int         lisa1_cs, lisa2_cs, ttlc_cs, dbg_cs;     // 0 or 1
    uint32_t    lisa1_base, lisa2_base, ttlc_base;
    int         is_flash[2], quad[2], addr16[2], dummy[2];
    int         spi_mode, sclk_div, ce_delay;
    int         cache_on, cache_map, shift_div;
    int         io_mux, out_mux;
    static LisaSetupCfg Defaults(void);
    std::vector<std::pair<uint8_t, uint16_t> > Registers(void) const;
    bool        FromRegisters(const std::map<uint8_t, uint16_t>& r);
    std::string Validate(void) const;                    // "" if consistent
    std::string Serialize(void) const;
    bool        Parse(const char *s);
};

typedef struct LisaCmd
{
   const char *      name;
   int               min_args;
   int               max_args;
   CLisaFunc_t       pFunc;
   const char *      usage;
   const char *      help;
} LisaCmd_t;

typedef struct lisa_program_s
{
  uint16_t           m_Program[32768];
  int                m_LstLine[32768];
  int                m_Size;
} lisa_program_t;

typedef struct lisa_symbol_s
{
   char *                     name;
   int                        line;
   struct lisa_symbol_s*    pNext;
} lisa_symbol_t;

typedef struct lisa_list_col_s
{
  char                name[16];
  short               col;
  short               len;
  int                 folded;
} lisa_list_col_t;

typedef struct lisa_src_line_s
{
  struct lisa_src_line_s  *pNext;
  char  *                  pLine;
} lisa_src_line_t;

typedef struct lisa_src_s lisa_src_t;
typedef struct lisa_src_s
{
  int                 type;      
  int                 attached;
  char                name[512];
  FILE *              fd;
  int                 topLine;
  int                 sourceLineCount;
  int                 lineStarts[90000];
  int                 lineAddrs[90000];
  lisa_symbol_t *     pSymbolList;
  lisa_list_col_t *   pListCols;
  lisa_program_t *    pProgram;
  lisa_src_line_t *   pFirstDynLine;
  int                 colCount;
  int                 headerLineno;
  char                filename[256];
  char                path[512];
  char                cdb_file[256];   // the .cdb's name for this source ("" if not one of its files)
  lisa_src_t *        pNext;
  CTab *              pTab;
} lisa_src_t;

class CLisa : public CTuiSource
{
public:
    CLisa(CTui *pTui);
    virtual ~CLisa();

    // TUI-required implementations
    int                 GetSourceLineCount(void *pCtx) override;
    void                DrawSourceWindow(void *pCtx, WINDOW* pWnd, int topLine, int lineCount) override;
    void                DrawWatchWindow(WINDOW* pWnd, int topLine) override;
    void                DrawWatchItem(WINDOW* pWnd, int line, const char *reg,
                                      const char *value, int colWidth = 12);
    void                DrawTermWindow(WINDOW* pWnd, int topLine, int lineCount);
    const TuiCmd_t *    GetCommandTable(void) override;
    int                 GetCommandTabList(char *pCmd, const char *pBuffer, TuiSortList_t *&pList) override;
    void                AddTuiSortItem(TuiSortList_t *pList, const char *pStr);
    TuiSortList_t     * BuildFileList(const char *pBuffer, bool isLsCmd, bool isRunCmd);
    void                FreeTabList(TuiSortList_t *pList) override;
    void                DebugPrintf(const char *fmt, ...) override;
    void                DrawSplash(WINDOW* pWnd);
    void                Printf(const char *fmt, ...);
    void                SingleStep(void);

    // Optional overrides
private:
    int                 Load(int argc, char* argv[]);
    int                 Verify(int argc, char* argv[]);
    // flashing (LisaFlash.cxx)
    int                 Exchange(const char *cmd, uint32_t& value, int timeoutMs = 500);
    bool                FlashIdle(int timeoutMs);
    int                 ProgramFlash(const std::vector<uint16_t>& words, uint32_t base);
    int                 VerifyFlash(const std::vector<uint16_t>& words, uint32_t base, bool quiet);
    int                 Run(int argc, char* argv[]);
    int                 Halt(int argc, char* argv[]);
    int                 Reset(int argc, char* argv[]);
    int                 Step(int argc, char* argv[]);
    int                 Connect(int argc, char* argv[]);
    int                 SP(int argc, char* argv[]);
    int                 PC(int argc, char* argv[]);
    int                 Read(int argc, char* argv[]);
    int                 Open(int argc, char* argv[]);
    int                 Attach(int argc, char* argv[]);
    int                 Break(int argc, char* argv[]);
    int                 Delete(int argc, char* argv[]);
    int                 Ls(int argc, char* argv[]);
    int                 Term(int argc, char* argv[]);
    int                 Write(int argc, char* argv[]);
    int                 Set(int argc, char* argv[]);
    int                 Help(int argc, char* argv[]);

    // Source level (LisaCdb.cxx): the image and its .cdb, the sources in
    // tabs, breakpoints by file:line, next/into/finish by running to
    // breakpoints, values from the registers and the stack
    int                 Debug(int argc, char* argv[]);
    int                 Next(int argc, char* argv[]);
    int                 Into(int argc, char* argv[]);
    int                 Finish(int argc, char* argv[]);
    int                 Print(int argc, char* argv[]);
    int                 Locals(int argc, char* argv[]);
    int                 Backtrace(int argc, char* argv[]);
    int                 Where(int argc, char* argv[]);
    int                 Setup(int argc, char* argv[]);
    int                 SetupDebugger(uint32_t flashBase);
    // the SETUP tab (LisaSetup.cxx)
    int                 ApplySetup(const LisaSetupCfg& cfg, std::string *pReport = NULL);
    bool                ReadSetup(LisaSetupCfg& cfg);
    void                OpenSetupTab(void);
    void                CloseSetupTab(void);
    bool                IsSetupTab(void);
    void                DrawSetupWindow(WINDOW *pWnd);
    int                 SetupKey(int key);
    void                SetDebugAddress(uint32_t byteAddr);
    bool                WantAllKeys(void) override { return IsSetupTab(); }
    void                SaveWatchItems(FILE *fd) override;
    void                RestoreOtherPref(char *pPref, char *pStr) override;
    int                 ReadRam(uint16_t addr, uint8_t *buf, int n);
    bool                ReadCore(uint32_t& pc, uint32_t& sp, uint32_t& ra, uint32_t& ix, uint32_t& acc);
    uint16_t            CodeWord(uint16_t pc);
    bool                WaitHalt(int timeoutMs);
    void                Grant(void);
    void                Reclaim(void);
    bool                RunGranted(int timeoutMs);
    uint16_t            SafeStop(uint16_t x);
    bool                RequireHalted(void);
    bool                SafeHalt(void);
    bool                LogicalCore(uint32_t& pc, uint32_t& sp, uint32_t& ra, uint32_t& ix, uint32_t& acc, bool *pShifted = NULL);
    int                 RunToStops(const std::vector<uint16_t>& stops, int timeoutMs);
    int                 StepInsn(void);
    int                 StepToLine(int mode);
    bool                ResolveLocation(const char *spec, uint16_t& addr, std::string& what);
    void                ShowLocation(bool activate);
    lisa_src_t *        SourceForFile(const std::string& file);
    bool                IsCdbSource(lisa_src_t *pSrc) { return pSrc->cdb_file[0] != 0; }
    int                 PCLineOf(lisa_src_t *pSrc, uint32_t pc);
    bool                IsBreakLine(lisa_src_t *pSrc, int line);
    lisa::Cdb::Frame    FrameHere(const lisa::CdbFunction **ppFn, uint32_t *pPc = NULL);
    static std::vector<uint16_t> NextPCs(uint16_t op, uint16_t pc, uint16_t ra, uint16_t ix);

    // LISA-specific interface
    int                 ReadReg(uint8_t reg, uint32_t &value);
    int                 WriteReg(uint8_t reg, uint32_t value);
    int                 ReadUartString(ser_params_t *pPort, char *buffer, int max_len, bool acquire=true,
                                    bool wantCrLf=false);
    int                 WriteUartString(const char *buffer);
    int                 CmdResponse(const char *cmd, const char *subResp,
                                 int retryCount, int sleepms);
    int                 AwaitString(const char *subResp, int retryCount, int sleepms);
    int                 ParseFile(lisa_src_t *pSrc, char *filename);
    int                 IsObjectFile(lisa_src_t *pSrc);
    int                 OpenFromListFile(lisa_src_t *pSrc, char *line, int maxlen);
    char *              GetLineToken(char *pLine, int col, int& syntax);
    void                AppendWS(char *pLine);
    void                CloseTab(CTab *pTab);
    void                DisassembleOpcode(uint32_t opcode, char *pStr, int len);
    void                ReadBreakpoints(void);
    void                BringPCOnWindow(void);
    void                ShortFlush(ser_params_t *pPort, bool acquire);
    bool                WantProcessKey(void);
    int                 WantFocus(void);
    void                LoseFocus(void *pCtx, WINDOW* pWnd);
    void                SetFocus(void *pCtx, WINDOW* pWnd, int topLine, int lineCount);
    bool                IsTermTab(void);
    int                 ProcessKey(int key);

private:
    ser_params_t*       m_pSer;
    int                 m_PortOpen;
    int                 m_Running;
    int                 m_HaltDetected;
    int                 m_Connected;
    int                 m_noSyntaxHilight;
    int                 m_Shuttle;
    bool                m_LisaHasUart;
    TuiSortList_t     * m_pCmdTabList;
    std::string         m_Response;
    char              * m_Paths[LISA_MAX_PATHS];
    lisa_src_t        * m_pSrcs;
    uint16_t            m_Breakpoints[4];
    char                m_token[256];
    char                m_TermLineText[512];
    int                 m_TermLineLen;
    CSimEvent           m_Access;
    CTui              * m_pTui;
    lisa_src_t        * m_pAttachedSrc;
    lisa_src_t        * m_pTermSrc;
    WINDOW            * m_pTermWin;
    int                 m_TermLine;
    int                 m_TermCol;
    int                 m_TermTopLine;
    int                 m_TermLineCount;
    lisa::Cdb           m_Cdb;
    std::vector<uint16_t> m_Program;        // the image `debug` loaded (code words, by PC)
    std::string         m_ProgramPath;
    uint16_t            m_Halted;           // the last halt seen by the source-level commands
    // stops moved past a store (the TT07 breakpoint hazard, Cdb::is_store):
    // the address planted -> { the address meant, the SP change between }
    std::map<uint16_t, std::pair<uint16_t, int> > m_Shift;
    std::string         m_OutLine;          // the program's output, a partial line
    bool                m_ImagesOnly;       // file completion offers program images only
    LisaSetupCfg        m_Setup;            // applied on connect, saved in .tui_prefs
    LisaSetupCfg        m_SetupEdit;        // the SETUP tab's copy until OK
    lisa_src_t        * m_pSetupSrc;
    int                 m_SetupField;       // the field with the focus
    bool                m_SetupFresh;       // the next digit starts a new number
    std::string         m_SetupStatus;
    static const LisaCmd_t m_TuiCmds[];
};

