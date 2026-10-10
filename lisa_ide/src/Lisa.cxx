// CLisa.cxx
#include "Lisa.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

extern int gLastWasCtrlC;

const LisaCmd_t CLisa::m_TuiCmds[] =
{
    {"attach",  0, 1, &CLisa::Attach,  "[shuttle]",     "Attach current file as running"},
    {"break",   0, 1, &CLisa::Break,   "[file:line|func|*addr]", "Set a breakpoint, or list them"},
    {"delete",  0, 1, &CLisa::Delete,  "[n]",           "Delete breakpoint n (as break lists them), or all"},
    {"br",      0, 1, &CLisa::Break,   "[file:line|func|*addr]", "Same as break"},
    {"ls",      0, 1, &CLisa::Ls,      "",              "Show directory listing"},
    {"load",    1, 2, &CLisa::Load,    "<prog.ihx> [base]", "Program the flash through LISA, verify it, then debug it"},
    {"verify",  1, 2, &CLisa::Verify,  "<prog.ihx> [base]", "Compare the flash with an image"},
    {"run",     0, 0, &CLisa::Run,     "",              "Run until a breakpoint or Ctrl-C (the program has the UART)"},
    {"halt",    0, 0, &CLisa::Halt,    "",              "Stop execution"},
    {"stop",    0, 0, &CLisa::Halt,    "",              "Stop execution"},
    {"step",    0, 1, &CLisa::Step,    "[n]",           "Step machine instructions"},
    {"s",       0, 1, &CLisa::Step,    "[n]",           "Step machine instructions"},
    {"set",     1, 2, &CLisa::Set,     "reg=val",       "Set a register value"},
    {"read",    1, 1, &CLisa::Read,    "addr",          "Read LISA register"},
    {"write",   2, 2, &CLisa::Write,   "addr value",    "Write LISA register"},
    {"reset",   0, 0, &CLisa::Reset,   "",              "Reset the LISA Core"},
    {"term",    0, 0, &CLisa::Term,    "",              "Enable the LISA Terminal"},
    {"open",    1, 1, &CLisa::Open,    "filename",      "Open a file for view in a tab"},
    {"connect", 1, 2, &CLisa::Connect, "<port> [baud]", "Connect to the board (its USB serial port)"},
    {"sp",      0, 0, &CLisa::SP,      "",              "Report current SP value"},
    {"pc",      0, 0, &CLisa::PC,      "",              "Report current PC value"},
    {"debug",   1, 1, &CLisa::Debug,   "<prog.ihx>",    "Load an image and its .cdb (sdcc --debug), open its sources"},
    {"next",    0, 0, &CLisa::Next,    "",              "Run to the next source line, over calls"},
    {"n",       0, 0, &CLisa::Next,    "",              "Run to the next source line, over calls"},
    {"into",    0, 0, &CLisa::Into,    "",              "Run to the next source line, into calls"},
    {"finish",  0, 0, &CLisa::Finish,  "",              "Run until the current function returns"},
    {"print",   1, 8, &CLisa::Print,   "<expr>",        "A variable: *, &, [i], .m, ->m"},
    {"p",       1, 8, &CLisa::Print,   "<expr>",        "A variable: *, &, [i], .m, ->m"},
    {"locals",  0, 0, &CLisa::Locals,  "",              "The current function's arguments and locals"},
    {"bt",      0, 0, &CLisa::Backtrace, "",            "Backtrace"},
    {"where",   0, 0, &CLisa::Where,   "",              "The source line at PC"},
    {"setup",   0, 1, &CLisa::Setup,   "[apply|defaults]", "The SETUP tab: chip selects, bases, SPI/QSPI, cache, muxes"},
    {"help",    0, 1, &CLisa::Help,    "[command]",     "List the commands, or explain one"},
    {nullptr,   0, 0, nullptr,         nullptr,         nullptr}
};

static const char *gDeclarator[] = 
{
   "#if",
   "#else",
   "#elif",
   "#endif",
   "#define",
   "#include",
   "set",
   "org",
   "end",
   "ts",
   "te",
   NULL
};

static const char *gScriptDecl[] = 
{
   ".public",
   ".extern",
   ".file",
   ".section",
   ".loc",
};

static const char *gKeywords[] = 
{
   "if",
   "else",
   "while",
   "for",
   "break",
   "continue",
   "return",
   "unsigned",
   "const",
   "accumulator",
   "register",
   "char",
   "short",
   "do",
   "goto",
   "push",
   "pop",
   "bit_set",
   "bit_clear",

   "andi", "and",  "adc",  "add",  "adx", "not",  "or",
   "sbc",  "sub",  "xor",  " xri", "sbb", "sbi",  "sbbi",  "ret",
   "lda",  "ldi",  "ldp",  "lxrb", "lxr", "lxi",  "move", "btst", "ifbc",
   "sxi",  "sxr",  "sxrb", "sta",  "stp", "spp",  "sir",  "air",  "br",
   "inc",  "dec",  "shl",  "shr",  "cmp", "cpi",  "mul",  "nop",  "wfe",
   "bz",   "bnz",  "call_ix", "ldax", "stax", "swap",
   "ifte", "iftt", "pop",  "push", "rets","reti", "ads", "jal",
   "inx",  "dcx",  "db",   "dw",  "ds", "ldx", "sra", "lra",
   "ldc",  "ldxx", "stxx", "push_ix", "xchg", "cpx", "call", "push_a", "pop_a",
   "xchg_sp", "xchg_ra", "xchg_ia",
   NULL
};

/*
==============================================================================
Lisa's constructor
==============================================================================
*/
CLisa::CLisa(CTui *pTui)
{
    int             count;
    int             x;
    TuiSortItem_t * pItem;
    TuiSortItem_t * pPrevItem;

    m_pTui            = pTui;
    m_pSer            = nullptr;
    m_PortOpen        = 0;
    m_Connected       = 0;
    m_noSyntaxHilight = 0;
    m_WorkingDir      = ".";
    m_PrefsFile       = ".tui_prefs";
    m_pAttachedSrc    = NULL;
    m_Running         = -1;
    m_HaltDetected    = 0;
    m_Shuttle         = 6;
    m_LisaHasUart     = false;
    m_TermLineLen     = 0;
    m_pTermSrc        = NULL;
    m_pTermWin        = NULL;
    m_TermLine        = 0;
    m_TermCol         = 0;
    m_Halted          = 0;
    m_ImagesOnly      = false;
    m_Setup           = LisaSetupCfg::Defaults();
    m_SetupEdit       = m_Setup;
    m_pSetupSrc       = NULL;
    m_SetupField      = 0;
    m_SetupFresh      = true;
    for (x = 0; x < 4; x++)
        m_Breakpoints[x] = 0;

    // Initialze the search paths
    for (x = 0; x < LISA_MAX_PATHS; x++)
        m_Paths[x] = NULL;

    m_Access.Open();
    m_Access.Release();

    // Add an initial path
    m_Paths[0] = (char *) malloc(16);
    strcpy(m_Paths[0], ".");

    // Create array of commands for tab expansion
    count = sizeof(m_TuiCmds) / sizeof(LisaCmd_t) - 1;
    m_pCmdTabList = new TuiSortList_t;
    m_pCmdTabList->pFirst = new TuiSortItem_t;
    pPrevItem = m_pCmdTabList->pFirst;
    pPrevItem->name = m_TuiCmds[0].name;
    pPrevItem->pNext = NULL;
  
    // Add all commands to the list
    for (x = 1; x < count; x++)
    {
        // Create a new item for this command
        pItem = new TuiSortItem_t;
        pItem->name = m_TuiCmds[x].name;
        pItem->pNext = NULL;
        pPrevItem->pNext = pItem;
        pPrevItem = pItem;
    }
}

/*
==============================================================================
Lisa's destructor
==============================================================================
*/
CLisa::~CLisa()
{
    if (m_PortOpen)
        ser_deinit(m_pSer);
    m_PortOpen = 0;
}

/*
==============================================================================
Lisa's internal printf function for printing to the TUI command window
==============================================================================
*/
// LISA_IDE_LOG=<file> appends everything printed here (for driving the
// IDE from a script); LISA_IDE_TRACE=<file> the register exchanges too
static FILE *ide_log(const char *env)
{
    static FILE *fds[2];
    static const char *names[2] = { "LISA_IDE_LOG", "LISA_IDE_TRACE" };
    int i = strcmp(env, names[0]) == 0 ? 0 : 1;
    if (!fds[i] && getenv(names[i]))
        fds[i] = fopen(getenv(names[i]), "a");
    return fds[i];
}

void CLisa::Printf(const char *fmt, ...)
{
    va_list   args;

    if (FILE *fd = ide_log("LISA_IDE_LOG"))
    {
        va_start(args, fmt);
        vfprintf(fd, fmt, args);
        va_end(args);
        fputc('\n', fd);
        fflush(fd);
    }
    va_start(args, fmt);
    m_pParent->UICommandPrintFormat(fmt, args);
    va_end(args);
}

/*
==============================================================================
Routine to write a string to the UART
==============================================================================
*/
int CLisa::WriteUartString(const char *buffer)
{
    m_Access.Acquire();
    for (size_t i = 0; i < strlen(buffer); ++i)
    {
        if (ser_write_byte(m_pSer, buffer[i]) != SER_NO_ERROR)
        {
            m_Access.Release();
            return -1;
        }
        usleep(100);
    }
    m_Access.Release();
    return 0;
}

/*
==============================================================================
Short UART flush
==============================================================================
*/
void CLisa::ShortFlush(ser_params_t *pPort,  bool acquire)
{
    int                 retry;
    struct  timespec    tm;
    char                buffer[2];
  
    // Loop for a while to read bytes
    retry = 0;
  
    if (acquire)
        m_Access.Acquire();
  
    // Skip leading \r\n characters
    while (retry < READ_RETRY_COUNT)
    {
        tm.tv_sec = 0;
        tm.tv_nsec = 10000000;
        nanosleep(&tm, NULL);
        ser_poll(pPort);
        if (ser_read_byte(pPort, buffer) == SER_NO_ERROR)
        {
            if (buffer[0] == 0x0d)
                break;
        }

        retry++;
    }

    if (acquire)
        m_Access.Release();
}
        
/*
==============================================================================
Routine to read a string from the UART
==============================================================================
*/
int CLisa::ReadUartString(ser_params_t *pPort, char *buffer, int max_len, bool acquire,
                          bool wantCrLf)
{
    int                 bytes, retry, ret;
    char                dummy = 0;
    char                ch;
    char                lastCh = 0;
    struct  timespec    tm;
    int                 crFound = 0;
    int                 lfCount = 0;
  
    // Loop for a while to read bytes
    retry = 0;
  
    if (acquire)
        m_Access.Acquire();
  
#if 0
    if (!m_LisaHasUart)
    {
        // Skip leading \r\n characters
        while (retry < READ_RETRY_COUNT)
        {
            tm.tv_sec = 0;
            tm.tv_nsec = 800000;
            nanosleep(&tm, NULL);
            ser_poll(pPort);
            
            dummy = 0;
            ret = ser_read_byte(pPort, &dummy);
            if (ret == SER_NO_ERROR)
            {
                // Skip any zero or -1 bytres
                if (dummy == 0 || dummy == -1)
                {
                    retry++;
                    continue;
                }
                
                // Test for 's' character
                if (dummy == 's')
                {
                    m_HaltDetected = 1;
                    continue;
                }
                
                // Skip newline characters
                if (dummy == '\r' || dummy == '\n')
                {
                    retry = 0;
                    continue;
                }
                
                break;
            }
            else
              retry++;
        }
    }
#endif
        
    bytes = 0;
    retry = 0;
    if (dummy != 0)
      buffer[bytes++] = dummy;
  
    while (retry < READ_RETRY_COUNT && bytes <= max_len-1)
    {
      ser_poll(pPort);
  
      ret = ser_read_byte(pPort, &buffer[bytes]);
      if (ret == SER_NO_ERROR)
      {
        // Skip any zero or -1 bytes
        if (buffer[bytes] == 0 || buffer[bytes] == -1)
        {
          retry++;
          continue;
        }

        ch = buffer[bytes];
        retry = 0;

        // Test for 's'top
        if (!m_LisaHasUart)
        {
            if (ch == 's' && !(lastCh == 'i'))
            {
                m_HaltDetected = 1;
                continue;
            }
        }
  
        // Stop if EOL character encountered
        if (ch == 0x0a)
        {
            if (m_Shuttle == 6)
            {
                if (wantCrLf)
                    buffer[++bytes] = 0;
                else
                    buffer[bytes++] = 0;
                if (acquire)
                    m_Access.Release();
                return bytes;
            }
            else
                crFound = 1;
        }
        else if ((ch == 0x0d) && crFound)
        {
          lfCount++;
          if (wantCrLf)
              buffer[++bytes] = 0;
          else
              buffer[bytes++] = 0;
          if (acquire)
              m_Access.Release();
          return bytes;
        }
  
        if (ch == 0x0d)
            lfCount++;
        if ((ch != 0x0a && ch != 0x0d) || wantCrLf)
            bytes++;

        lastCh = ch;
      }
      else
      {
          tm.tv_sec = 0;
          tm.tv_nsec = 10000000;
          nanosleep(&tm, NULL);
          retry++;
      }
    }
  
    if (retry >= READ_RETRY_COUNT)
    {
      if (acquire)
          m_Access.Release();
      return 0;
    }
  
    // NULL Terminate any received string
    buffer[bytes] = 0;
    if (acquire)
        m_Access.Release();
    return bytes;
}

/*
==============================================================================
Read a LISA register using the debug interface
==============================================================================
*/
int CLisa::ReadReg(uint8_t reg, uint32_t& value)
{
    char cmd[16];
    char buffer[32];

    m_Access.Acquire();

    snprintf(cmd, sizeof(cmd), "r%02x\n", reg);
    for (const char* p = cmd; *p; ++p)
        ser_write_byte(m_pSer, *p);

    usleep(30000);

    // The reply is four hex digits on a line of their own; an echo of an
    // earlier newline (the debugger answers each with \n\r) may precede it.
    int   ret = -1;
    FILE *trace = ide_log("LISA_IDE_TRACE");
    for (int attempt = 0; attempt < 4 && ret == -1; attempt++)
    {
        memset(buffer, 0, sizeof(buffer));
        int n = ReadUartString(m_pSer, buffer, sizeof(buffer), false);
        if (trace)
            fprintf(trace, "r%02x -> %d %s\n", reg, n, buffer);
        if (n <= 0)
            break;
        for (int i = 0; buffer[i]; i++)
            if (isxdigit((unsigned char) buffer[i]) && isxdigit((unsigned char) buffer[i+1]) &&
                isxdigit((unsigned char) buffer[i+2]) && isxdigit((unsigned char) buffer[i+3]) &&
                !isxdigit((unsigned char) buffer[i+4]))
            {
                value = strtoul(&buffer[i], nullptr, 16);
                ret = 0;
                break;
            }
    }
    if (trace)
        fflush(trace);
    m_Access.Release();
    return ret;
}

/*
==============================================================================
Write a LISA register using the debug interface
==============================================================================
*/
int CLisa::WriteReg(uint8_t reg, uint32_t value)
{
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "w%02x%04x\n", reg, value);
    m_Access.Acquire();

    // Write the command bytes
    for (const char* p = cmd; *p; ++p)
        ser_write_byte(m_pSer, *p);

    // no reply, only the newline's echo (\n\r)
    int n = ReadUartString(m_pSer, cmd, sizeof(cmd), false);
    if (FILE *trace = ide_log("LISA_IDE_TRACE"))
    {
        fprintf(trace, "w%02x %04x -> %d %s\n", reg, value, n, cmd);
        fflush(trace);
    }

    m_Access.Release();
    return 0;
}

/*
==============================================================================
Draw an item on the watch window
==============================================================================
*/
void CLisa::DrawWatchItem(WINDOW* pWnd, int line, const char *reg, 
                            const char *value, int colWidth)
{
    wmove(pWnd, line, 1);
    wclrtoeol(pWnd);
    wattron(pWnd, COLOR_PAIR(SYNTAX_PAIR_LINENO));
    mvwprintw(pWnd, line, 1, "%s", reg);
    wattron(pWnd, COLOR_PAIR(SYNTAX_PAIR_NORMAL));
    mvwprintw(pWnd, line, colWidth-1, "%s", value);
}

/*
==============================================================================
Draw the Lisa watch window
==============================================================================
*/
void CLisa::DrawWatchWindow(WINDOW* pWnd, int topLine)
{
    const char* labels[] = {"PC", "SP", "ACC", "IX", "RA", "FLAGS", "STATUS"};
    uint8_t     regs[] = {REG_PC, REG_SP, REG_ACC, REG_IX, REG_RA, REG_FLAGS, REG_STATUS};
    uint32_t    value;
    char        str[32];
    char        valStr[128];
    uint32_t    pc;
    int         i;

    if (!m_Connected)
        return;

    werase(pWnd);
    // Get the PC
    ReadReg(2, pc);
    snprintf(str, sizeof(str), "%-12s", labels[0]);
    snprintf(valStr, sizeof(valStr), "0x%04X", pc);
    DrawWatchItem(pWnd, topLine, str, valStr);

    // Draw the remaining items
    for (i = 1; i < 7; ++i)
    {
        snprintf(str, sizeof(str), "%-12s", labels[i]);

        if (ReadReg(regs[i], value) == 0)
        {
            if (strcmp(labels[i], "FLAGS") == 0)
            {
                int zf = (value >> 8) & 1;
                int cf = (value >> 9) & 1;

                snprintf(valStr, sizeof(valStr), "C:%d  Z:%d", cf, zf);
            }
            else if (regs[i] == REG_ACC)
            {
                value &= 0xFF;
                snprintf(valStr, sizeof(valStr), "0x%02X", value);
            }
            else
                snprintf(valStr, sizeof(valStr), "0x%04X", value);

            DrawWatchItem(pWnd, topLine + i, str, valStr);

            if (regs[i] == REG_FLAGS && (value & 1) && m_Running)
            {
                m_Running = 0;
                m_HaltDetected = 1;
            }
        }
        else
            DrawWatchItem(pWnd, topLine + i, str, "????");
    }

    // Get the opcode at PC
    WriteReg(0x10, pc*2);
    ReadReg(0x20, value);
    snprintf(str, sizeof(str), "%-12s", "OPCODE");
    snprintf(valStr, sizeof(valStr), "0x%04X", value);
    DrawWatchItem(pWnd, topLine + i++, str, valStr);

    // Disassemble the opcode
    DisassembleOpcode(value, valStr, sizeof(valStr));
    snprintf(str, sizeof(str), "%-12s", "Instr");
    DrawWatchItem(pWnd, topLine + i++, str, valStr);

    // Refresh the window
    wrefresh(pWnd);

    // Test if Halt detected
    if (m_HaltDetected)
    {
        m_HaltDetected = 0;
        m_pParent->DrawSourceWindow();
    }
}

/*
==============================================================================
Parse the next token from the line and return a pointer to it, plus provide
the syntax highlite type.
==============================================================================
*/
char * CLisa::GetLineToken(char *pLine, int col, int& syntax) 
{
    int      x;
    char     *ptr;
 
    // Test for end of string
    if (*pLine == '\0')
        return NULL;
 
    ptr = pLine;
    if (col == 0)
    {
        // Test for label / function
        for (x = 0; *ptr != '\0' && *ptr != ' ' && *ptr != '(' && *ptr != ':'; x++)
            ptr++;
        
        // Test for label or function
        if (*ptr == '(' || *ptr == ':')
        {
            // Copy label to our member string
            strncpy(m_token, pLine, x);
            m_token[x] = 0;
            syntax = SYNTAX_PAIR_LABEL;
            return m_token;
        }
    } 
 
    // Test for keywords
    for (x = 0; gKeywords[x] != NULL; x++)
    {
       // Compare this keyword
       if (strncmp(gKeywords[x], pLine, strlen(gKeywords[x])) == 0)
       {
          int len = strlen(gKeywords[x]);
 
          // Validate next byte isn't alnum
          if (!isalnum(pLine[len]) && pLine[len] != '_')
          {
             // Copy keyword to m_token
             strcpy(m_token, gKeywords[x]);
             AppendWS(pLine);
 
             // Set the syntax type
             syntax = SYNTAX_PAIR_KEYWORD;
             return m_token;
          }
       }
    }
 
    // Test for script declarator
    for (x = 0; gScriptDecl[x] != NULL; x++)
    {
       // Compare this keyword
       if (strncmp(gScriptDecl[x], pLine, strlen(gScriptDecl[x])) == 0)
       {
          int len = strlen(gScriptDecl[x]);
 
          // Validate next byte isn't alnum
          if (!isalnum(pLine[len]) && pLine[len] != '_')
          {
             // Copy keyword to m_token
             strcpy(m_token, gScriptDecl[x]);
             AppendWS(pLine);
 
             // Set the syntax type
             syntax = SYNTAX_PAIR_REG;
             return m_token;
          }
       }
    }
 
    // Test for declarator
    for (x = 0; gDeclarator[x] != NULL; x++)
    {
       // Compare this keyword
       if (strncmp(gDeclarator[x], pLine, strlen(gDeclarator[x])) == 0)
       {
          int len = strlen(gDeclarator[x]);
 
          // Validate next byte isn't alnum
          if (!isalnum(pLine[len]) && pLine[len] != '_')
          {
             // Copy keyword to m_token
             strcpy(m_token, gDeclarator[x]);
             AppendWS(pLine);
 
             // Set the syntax type
             syntax = SYNTAX_PAIR_DECLARATOR;
             return m_token;
          }
       }
    }
 
    // Test for whitespace.  Simply print it using normal syntax
    if (*pLine == ' ' || *pLine == '\t')
    {
       // Copy whitespace to our token
       for (x = 0; pLine[x] == ' ' || pLine[x] == '\t'; x++)
       {
          m_token[x] = pLine[x];
       }
       m_token[x] = 0;
       syntax = SYNTAX_PAIR_NORMAL;
       return m_token;
    }
 
    // Test for comment
    if ((pLine[0] == '/' && pLine[1] == '/') || pLine[0] == '#')
    {
       syntax = SYNTAX_PAIR_COMMENT;
       return pLine;
    }
 
    // Test for string
    if (*pLine == '"' || *pLine == '\'')
    {
       // Copy opening quote character to m_token
       m_token[0] = *pLine;
       for (x = 1; pLine[x] != *pLine && pLine[x] != 0 ; x++)
       {
          // Test for escaped character
          if (pLine[x] == '\\')
          {
             // Copy the '\\' in case we are escaping a quote
             m_token[x] = pLine[x];
             x++;
          }
 
          // Copy the character to m_token
          m_token[x] = pLine[x];
       }
 
       // Test if end of line found prior to close quote, copy closing
       // quote if not
       if (pLine[x] != 0)
       {
          m_token[x] = pLine[x];
          x++;
       }
       m_token[x] = 0;
 
       AppendWS(pLine);
       syntax = SYNTAX_PAIR_DECLARATOR;
       return m_token;
    }
 
    // Test for punctuation
    if (ispunct(*pLine))
    {
       // Copy all repeated punctuation to m_token
       for (x = 0; ispunct(pLine[x]) && pLine[x] != '\'' && pLine[x] != '"'; x++)
          m_token[x] = pLine[x];
       m_token[x] = 0;
 
       // Now append any whitespace
       AppendWS(pLine);
       
       // Set the syntax rule and return
       syntax = SYNTAX_PAIR_NORMAL;
       return m_token;
    }
 
    // Test for numeric value
    if (isdigit(*pLine))
    {
       // Copy all digit characters to token, including 'x'
       for (x = 0; isxdigit(pLine[x]) || pLine[x] == 'x'; x++)
          m_token[x] = pLine[x];
       m_token[x] = 0;
 
       AppendWS(pLine);
       syntax = SYNTAX_PAIR_DECLARATOR;
       return m_token;
    }
 
    // Test for normal text
    if (isalnum(*pLine))
    {
       // Copy all alnum characters to token
       for (x = 0; isalnum(pLine[x]) || pLine[x] == '_'; x++)
          m_token[x] = pLine[x];
       m_token[x] = 0;
 
       AppendWS(pLine);
       syntax = SYNTAX_PAIR_NORMAL;
       return m_token;
    }
 
    syntax = SYNTAX_PAIR_NORMAL;
    return pLine;
}

/*
==============================================================================
Append whitespace from the given line pointer to the m_token variable based on 
current data already copied from pLine to m_token
==============================================================================
*/
void CLisa::AppendWS(char *pLine) 
{
    int      idx;
    
    // Get index based on string length
    idx = strlen(m_token);
    while (*pLine == ' ' || *pLine == '\t')
    {
        m_token[idx++] = *pLine++;
    }
    
    // Zero terminate
    m_token[idx] = 0;
}

/*
==============================================================================
Process the keys for TERM tab
==============================================================================
*/
int CLisa::ProcessKey(int key)
{
    char    buffer[512];
    char    ch;
    int     bytes;
    int     x;

    // the SETUP tab's form takes the keys while it has the focus
    if (IsSetupTab())
        return SetupKey(key);

    // Validate we have a good Term Src pointer
    if (m_pTermSrc == NULL)
        return 0;

    // Prepare to send key to LISA
    buffer[0] = key;

    // Send the key to LISA
    ser_write_byte(m_pSer, buffer[0]);

    // Read any response from LISA
    do {
        // Read bytes from the UART
        bytes = ReadUartString(m_pSer, buffer, sizeof(buffer), true, true);

        // If we receive bytes, process them into lines
        if (bytes > 0)
        {
            // Loop for all received bytes
            for (x = 0; x < bytes; x++)
            {
                // Get the next byte received
                ch = buffer[x];

                // Test for \r\n
                if (ch == '\r' || ch == '\n')
                {
                    // Add a new line to the source
                    if (ch == '\n')
                    {
                        m_TermLine++;
                        m_TermCol = 0;
                        wmove(m_pTermWin, m_TermLine, m_TermCol);
                    }
                }
                else
                {
                    // Add the character to the window
                    if (m_pTermWin)
                    {
                        mvwaddch(m_pTermWin, m_TermLine, m_TermCol, ch);
                        m_TermCol++;
                        wmove(m_pTermWin, m_TermLine, m_TermCol);
                        wrefresh(m_pTermWin);
                    }
                }
            }
        }
    } while (bytes > 0);
    
    return 1;
}

/*
==============================================================================
Draw the TERM window contents
==============================================================================
*/
void CLisa::DrawTermWindow(WINDOW* pWnd, int topLine, int lineCount)
{
    werase(pWnd);
    wrefresh(pWnd);
}

/*
==============================================================================
Let TUI know if we want to process keys
==============================================================================
*/
bool CLisa::IsTermTab(void)
{
    CTab    *pTab;
    lisa_src_t  *pSrc;

    pTab = m_pTui->GetActiveSrcTab();
    if (pTab)
    {
        // Find the source associated with the active tab
        pSrc = m_pSrcs;

        // Scan all sources
        while (pSrc)
        {
            if (pSrc->pTab == pTab)
                break;
            pSrc = pSrc->pNext;
        }

        // Test if source found
        if (pSrc == NULL)
            return false;

        if (pSrc->type == LISA_SRC_TYPE_TERM)
            return true;
    }

    return false;
}

/*
==============================================================================
Let TUI know if we want the focus
==============================================================================
*/
int CLisa::WantFocus(void)
{
    return IsTermTab() || IsSetupTab() ? 1 : 0;
}

/*
==============================================================================
Called when the tab loses the focus
==============================================================================
*/
void CLisa::SetFocus(void *pCtx, WINDOW* pWnd, int topLine, int lineCount)
{
    lisa_src_t  *pSrc = (lisa_src_t *) pCtx;

    // Check if the focus tab is Term tab
    if (pSrc && pSrc->type == LISA_SRC_TYPE_TERM)
    {
        if (!m_LisaHasUart)
        {
            // Ensure the Watch window is inactive
            m_pTui->m_WatchPaused = 1;
            usleep(300000);
            WriteUartString("l");
            m_LisaHasUart = true;
        }

        m_pTermWin = pWnd;
        m_TermTopLine = topLine;
        m_TermLineCount = lineCount;
    }
}

/*
==============================================================================
Called when the tab loses the focus
==============================================================================
*/
void CLisa::LoseFocus(void *pCtx, WINDOW* pWnd)
{
    CTab        *pTab;
    lisa_src_t  *pSrc = (lisa_src_t *) pCtx;
    char        buffer[64];

    pTab = pSrc->pTab;
    if (pTab)
    {
        if (pSrc->type == LISA_SRC_TYPE_TERM && m_LisaHasUart)
        {
            // Send the hayes +++ sequence when we lose focus
            WriteUartString("+++");
            usleep(1000000);  // wait for state change
            ReadUartString(m_pSer, buffer, sizeof(buffer));
            m_LisaHasUart = false;
        }

        m_pTui->m_WatchPaused = 0;
    }
}

/*
==============================================================================
Let TUI know if we want to process keys
==============================================================================
*/
bool CLisa::WantProcessKey(void)
{
    return IsTermTab();
}

/*
==============================================================================
Draw's source windows
==============================================================================
*/
void CLisa::DrawSourceWindow(void *pCtx, WINDOW* pWnd, int topLine, int lineCount)
{
    char          line[256];
    char          raw[128];
    char          str[256];
    char          line_exp[8];
    int           x;
    int           addr = topLine - 1;
    int           c;
    int           i;
    int           col;
    int           pcLine;
    char         *pToken;
    int           syntax;
    uint32_t      regVal;
  
    lisa_src_t *pSrc = (lisa_src_t *) pCtx;
  
    // Test for TERM tab
    if (pSrc->type == LISA_SRC_TYPE_TERM)
    {
        DrawTermWindow(pWnd, topLine, lineCount);
        return;
    }
    if (pSrc->type == LISA_SRC_TYPE_SETUP)
    {
        DrawSetupWindow(pWnd);
        return;
    }

    // Open the file
    if (pSrc->fd == NULL)
        return;
  
    // Erase the screen
    werase(pWnd);
  
    // Get the PC line number if this src is attached
    pcLine = 0;
    if (pSrc->attached)
    {
        // Test if running
        if (ReadReg(0, regVal) != -1)
        {
            // IF we are halted...
            if (regVal & 1)
            {
                // Get the PC value
                ReadReg(2, regVal);
  
                // Find the line number of this address
                for (c = topLine; c < pSrc->sourceLineCount && c < topLine + lineCount; c++)
                    if (pSrc->lineAddrs[c] == (int) regVal)
                    {
                        pcLine = c+1;
                        break;
                    }
            }
        }
    }
  
    // A source of the .cdb: the PC's line from the debug info
    if (!pcLine && IsCdbSource(pSrc) && m_Connected && !m_LisaHasUart &&
        ReadReg(0, regVal) != -1 && (regVal & 1) && ReadReg(2, regVal) != -1)
        pcLine = PCLineOf(pSrc, regVal);

    pSrc->topLine = topLine;
    fseek(pSrc->fd, pSrc->lineStarts[topLine-1], SEEK_SET);

    for (x = 0; x < lineCount; x++)
    {
        // Print the line number to the string (a '*' marks a breakpoint's line)
        snprintf(line, sizeof(line), "%-5d%s%c", topLine+x, topLine+x==pcLine?"->":"  ",
                 IsCdbSource(pSrc) && IsBreakLine(pSrc, topLine+x) ? '*' : ' ');
       
        if (pSrc->type == LISA_SRC_TYPE_OBJ)
        {
            uint32_t    opcode;
       
            if (fgets(raw, sizeof(raw), pSrc->fd) == NULL)
                break;
       
            addr = pSrc->lineAddrs[topLine + x-1];
            opcode = strtol(raw, NULL, 16);
            DisassembleOpcode(opcode, raw, sizeof(raw));
            snprintf(&line[8], sizeof(line) - 8, "0x%04x:  0x%04x  %s", addr, opcode, raw);
       
            // For LDX opcode, we must get the next word from the file
            if (opcode == 0xA180)
            {
                if (fgets(raw, sizeof(raw), pSrc->fd) == NULL)
                    break;
                opcode = strtol(raw, NULL, 16);
                sprintf(raw, "       0x%04x", opcode);
                strcat(line, raw);
                addr++;
            }
        }
        else
        {
            // Read line from the file
            if (fgets(&line[8], sizeof(line)-8, pSrc->fd) == NULL)
              break;
        }
       
        // Remove trailing \r\n
        c = strlen(line);
        while (line[c-1] == '\n' || line[c-1] == '\r')
            line[--c] = '\0';
       
        // Expand tabs in-place
       
        if (m_noSyntaxHilight)
        {
            // Print the line with no color
            line[getmaxx(pWnd)-2] = '\0';
            mvwprintw(pWnd, x, 1, "%s", line);
            continue;
        }
       
        // Draw the lineno in a different color
        strncpy(line_exp, line, 5);
        line_exp[5] = 0;
        wattron(pWnd, COLOR_PAIR(SYNTAX_PAIR_LINENO));
        mvwprintw(pWnd, x, 1, "%s", line_exp);
        wattron(pWnd, COLOR_PAIR(SYNTAX_PAIR_NORMAL));
       
        // Test for ->
        if (line[5] == '-' && (pSrc->type == LISA_SRC_TYPE_OBJ ||
                    pSrc->type == LISA_SRC_TYPE_LIST || IsCdbSource(pSrc)))
        {
            mvwprintw(pWnd, x, 6, "->");
        }
       
        // Start a col 5, right after the line number
        col = 8;
       
        // Parse out syntax tokens from the line and print each separately
        // with the appropriate color.
        while ((pToken = GetLineToken(&line[col], col-8, syntax)) != NULL)
        {
            // Set the syntax color pair and print the token
            wattron(pWnd, COLOR_PAIR(syntax));
            if (col+2 + (int) strlen(pToken) > getmaxx(pWnd))
                pToken[getmaxx(pWnd) - (col+2)] = 0;
           
            // Test if token has embedded % which we must escape to prevent
            // the print routine from intrepreting as a format character
            c = 0;
            i = 0;
            while (pToken[i] != 0)
            {
                if (pToken[i] == '%')
                    str[c++] = '%';
                str[c++] = pToken[i++];
            }
            str[c] = 0;
            mvwprintw(pWnd, x, col+1, "%s", str);
            wattroff(pWnd, COLOR_PAIR(syntax));
           
            // Advance the col
            col += strlen(pToken);
            if (col+2 >= getmaxx(pWnd))
                break;
        }
    }
  
    wattron(pWnd, COLOR_PAIR(SYNTAX_PAIR_NORMAL));
}

/*
==============================================================================
Get the line count of specified source
==============================================================================
*/
int CLisa::GetSourceLineCount(void *pCtx)
{
    lisa_src_t *pSrc = (lisa_src_t *) pCtx;
    
    return pSrc->sourceLineCount; 
}

/*
==============================================================================
Close a tab
==============================================================================
*/
void CLisa::CloseTab(CTab *pTab)
{
    void            *pCtx;
    lisa_src_t      *pSrc;
    lisa_src_t      *pListSrc;
    lisa_symbol_t   *pSymbol;
    lisa_symbol_t   *pNextSymbol;
  
    pCtx = pTab->SourceContext();
    pSrc = (lisa_src_t *) pCtx;
    if (pSrc == m_pTermSrc)
        m_pTermSrc = NULL;
    if (pSrc == m_pSetupSrc)            // closed with F4: the edits are dropped
        m_pSetupSrc = NULL;

    // Free the list source control items
    if (pSrc->pListCols)
        free(pSrc->pListCols);
    if (pSrc->pProgram)
        free(pSrc->pProgram);
    if (pSrc->pSymbolList)
    {
        // Free all memory for symbols
        pSymbol = pSrc->pSymbolList;
        while (pSymbol != NULL)
        {
            // Get pointer to next symbol
            pNextSymbol = pSymbol->pNext;
            free(pSymbol);
            pSymbol = pNextSymbol;
        }
    }

    // Remove the source from the linked list
    if (pSrc == m_pSrcs)
    {
        // Simply make pSrc->pNext the first item in the list
        m_pSrcs = pSrc->pNext;
    }
    else
    {
        // Find the source in the list
        pListSrc = m_pSrcs;
        while (pListSrc)
        {
            if (pListSrc->pNext == pSrc)
            {
                // Remove it from the list
                pListSrc->pNext = pSrc->pNext;
                break;
            }
            
            // Point to next source
            pListSrc = pListSrc->pNext;
        }
    }

    // Free the source memory
    free(pSrc);

    // Now delete the tab and NULL out our pointer
    m_pParent->DeleteTab(pTab);
}

const TuiCmd_t* CLisa::GetCommandTable()
{
    return (TuiCmd_t *) &m_TuiCmds[0];
}

/*
==============================================================================
Build a TuiSortList of all known variables for tab expansion.
==============================================================================
*/
TuiSortList_t * CLisa::BuildFileList(const char *pBuffer, bool isLsCmd, bool isRunCmd)
{
    TuiSortList_t     *pList;
    DIR               *dir = NULL;
    struct dirent     *dp;
    char              name[257];
    char             *pBuf;
    char              ch;
    int               len;
    bool              usepath = !isLsCmd;
    char              **pPaths;
    int               pathcount;
    int               p;
  
    // Create new TuiSortList
    pList = new TuiSortList_t;
    if (pList)
    {
        // NULL the first item entry in the list
        pList->pFirst = NULL;
        
        // Test for opening a subdir
        len = strlen(pBuffer);
        if (len > 0)
        {
            // Rewind to first directory char
            while (len > 0 && pBuffer[len-1] != '/' &&
                pBuffer[len-1] != '\\')
            {
                len--;
            }
            
            // Test if directory character found
            if (pBuffer[len-1] == '\\' || pBuffer[len-1] == '/')
            {
                // Replace the '/' char with NULL so the opendir
                // only tries to open the actual directory, just
                // in case there is a partial file specified after
                // the directory.
                pBuf = (char *) &pBuffer[len-1];    
                ch = *pBuf;
                *pBuf = 0;
              
                // Open the directory and restore the '/' char
                dir = opendir(pBuffer);
                *pBuf = ch;
                usepath = 0;
            }
        }
        
        // If no directory opened above, then open current dir
        p = 0;
        if (dir != NULL)
        {
            pPaths = NULL;
            pathcount = 1;
        }
        else if (!usepath)
        {
            dir = opendir(".");
            pPaths = NULL;
            pathcount = 1;
        }
        else
        {
            dir = opendir(*m_Paths);
            pPaths = m_Paths;
            pathcount = LISA_MAX_PATHS;
        }
        
        while (pathcount)
        {
            if (dir != NULL)
            {
                // Now read all entries from the directory
                while ((dp = readdir(dir)) != NULL)
                {
                    // Not all filesystems fill out d_type
                    if (dp->d_type == DT_UNKNOWN)
                    {
                        struct stat st;
                        
                        if (fstatat(dirfd(dir), dp->d_name, &st, 0) == 0)
                        {
                            if (S_ISDIR(st.st_mode))
                                dp->d_type = DT_DIR;
                            else if (S_ISREG(st.st_mode))
                                dp->d_type = DT_REG;
                            else if (S_ISBLK(st.st_mode))
                                dp->d_type = DT_BLK;
                            else if (S_ISCHR(st.st_mode))
                                dp->d_type = DT_CHR;
                        }
                    }
                    
                    // Get name length
                    len = strlen(dp->d_name);

                    // program images only (load, debug, verify): directories and .ihx/.hex
                    if (m_ImagesOnly && dp->d_type != DT_DIR &&
                        !(len > 4 && (strcasecmp(&dp->d_name[len-4], ".ihx") == 0 ||
                                      strcasecmp(&dp->d_name[len-4], ".hex") == 0)))
                        continue;
                    if (m_ImagesOnly && dp->d_type == DT_DIR && dp->d_name[0] == '.' &&
                        strcmp(dp->d_name, "..") != 0)
                        continue;
                    
                    // Copy the name to a local var so we can append
                    // a '/' char after directory names
                    strcpy(name, dp->d_name);
                    if (dp->d_type == DT_DIR)
                        strcat(name, "/");
                    
                    AddTuiSortItem(pList, name);
                }
            }
            
            if (dir != NULL)                // a directory that would not open: nothing to list
                closedir(dir);
            if (pathcount == 0 || pPaths == NULL || pPaths[p] == NULL)
                break;
            
            dir = opendir(pPaths[p]);
            pathcount--;
            p++;
        }
    }
  
    return pList;
}

/*
==============================================================================
Add an item to a TuiSortList
==============================================================================
*/
void CLisa::AddTuiSortItem(TuiSortList_t *pList, const char *pStr)
{
    TuiSortItem_t *pItem;
    TuiSortItem_t *pCurrItem;
    TuiSortItem_t *pPrevItem;
  
    // Don't add duplicates
    pItem = pList->pFirst;
    while (pItem != NULL)
    {
        // Test if this symbol matches
        if (strcmp(pItem->name, pStr) == 0)
            return;
    
        // Get next symbol in the table
        pItem = pItem->pNext;
    }
  
    // First create a new TuiSortItem
    pItem = (TuiSortItem_t *) malloc(sizeof(TuiSortItem_t) + strlen(pStr) + 1);
    pItem->name = ((char *) pItem) + sizeof(TuiSortItem_t);
    strcpy((char *) pItem->name, pStr);
    DebugPrintf("%s\n", pItem->name);
  
    // Now insert the pItem into the list
    pCurrItem = pList->pFirst;
    pPrevItem = NULL;
    while (pCurrItem != NULL)
    {
        // Test if symbol name is greater than current symbol
        if (strcmp(pItem->name, pCurrItem->name) > 0)
        {
            pPrevItem = pCurrItem;
            pCurrItem = pCurrItem->pNext;
            continue;
        }
       
        // Insert symbol here
        break;
    }
  
    // Test if pCurrSymbol is NULL
    if (pPrevItem == NULL)
    {
        // Insert at beginning of list
        pItem->pNext = pList->pFirst;
        pList->pFirst = pItem;
    }
    else
    {
        // Insert somwhere in the middle of the list
        pItem->pNext = pPrevItem->pNext;
        pPrevItem->pNext = pItem;
    }
}

/*
==============================================================================
Get the list of commands for tab completion.
==============================================================================
*/
int CLisa::GetCommandTabList(char *pCmd, const char *pBuffer, TuiSortList_t*& pList)
{
    int     c;

    // If no command active yet, then return a reference to the command table
    if (pCmd == NULL || (pCmd && strlen(pCmd) == 0) || 
        (pCmd && strcmp(pCmd, "help") == 0))
    {
        pList = m_pCmdTabList;
        return 1;
    }

    // Test for commands that use function names as arguments
    if (strcmp(pCmd, "open") == 0 || strcmp(pCmd, "ls") == 0 ||
        strcmp(pCmd, "connect") == 0 || strcmp(pCmd, "load") == 0 ||
        strcmp(pCmd, "debug") == 0 || strcmp(pCmd, "verify") == 0)
    {
        // Skip past filename in the buffer
        c = strlen(pCmd);
        while (pBuffer[c] == ' ')
          c++;
        
        // Build a list of known variables
        // load / debug / verify take a program image: offer only those and directories
        m_ImagesOnly = strcmp(pCmd, "load") == 0 || strcmp(pCmd, "debug") == 0 || strcmp(pCmd, "verify") == 0;
        pList = BuildFileList(&pBuffer[c], strcmp(pCmd, "ls") == 0 ||
                  strcmp(pCmd, "path") == 0, strcmp(pCmd, "run") == 0);
        m_ImagesOnly = false;
        return 1;
    }

    pList = nullptr;
    return 0;
}

/*
==============================================================================
Free the previously allocated tab array (provided).
==============================================================================
*/
void CLisa::FreeTabList(TuiSortList_t *pList)
{
    TuiSortItem_t   *pItem;
    TuiSortItem_t   *pNext;
  
    // Don't free our command tab array
    if (pList != m_pCmdTabList)
    {
        // Free all items in the list
        pItem = pList->pFirst;
        while (pItem != NULL)
        {
            // Get pointer to the next item
            pNext = pItem->pNext;
            free(pItem);
            
            // Advance to next item
            pItem = pNext;
        }
       
        // Delete the list
        delete pList;
    }
}

void CLisa::DebugPrintf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
//    vfprintf(stderr, fmt, args);
    va_end(args);
}

/*
==============================================================================
Run the LISA core
==============================================================================
*/
int CLisa::Run(int argc, char* argv[])
{
    if (!m_Connected)
        return 1;

    // The program has the UART while it runs (its putc would otherwise wait
    // for a TX-ready that never comes), and the core is looked at between
    // its output; until a breakpoint, a brk or Ctrl-C.  Off a breakpoint at
    // the PC by one instruction first.
    Printf("running: Ctrl-C halts");
    int pc = RunToStops(std::vector<uint16_t>(), 0x7fffffff);
    if (pc < 0)
        Printf("the core did not halt");
    if (m_pAttachedSrc)
        BringPCOnWindow();
    ShowLocation(true);
    return 0;
}

/*
==============================================================================
Ensure the PC line is visible
==============================================================================
*/
void CLisa::BringPCOnWindow(void)
{
    uint32_t    pc;
    int         x;
    int         top;
    int         count;
    lisa_src_t  *pSrc = m_pAttachedSrc;

    if (ReadReg(2, pc) != -1)
    {
        // Find the source line
        for (x = 0; x < pSrc->sourceLineCount; x++)
        {
            if (pSrc->lineAddrs[x] == (int) pc)
            {
                CTab *pTab = pSrc->pTab;

                top = pTab->SourceFirstLine();
                count = pTab->SourceWindowLineCount();

                if (x+1 >= top && x+1 < top+count)
                    return;

                int line = x+1 - count/2;
                if (line < 1)
                    line = 1;
                pTab->SourceFirstLine(line);
                break;
            }
        }
    }
}

/*
==============================================================================
Halt the LISA core
==============================================================================
*/
int CLisa::Halt(int argc, char* argv[])
{
    char        buffer[5];
    lisa_src_t  *pSrc = m_pAttachedSrc;

    // Halt the processor
    if (!m_Connected)
        return 0;

    // Halt the core: by a breakpoint on the loop it is running (SafeHalt),
    // not asynchronously, so no store is lost (TT07); the breakpoint
    // registers are put back afterwards
    {
        uint32_t saved[4];
        for (int i = 0; i < 4; i++)
            ReadReg(8 + i, saved[i]);
        SafeHalt();
        for (int i = 0; i < 4; i++)
            WriteReg(8 + i, saved[i]);
    }
    ShowLocation(true);

    // Check for 's' break signal
    ReadUartString(m_pSer, buffer, sizeof(buffer));
    m_HaltDetected = 0;
    m_Running = 0;

    // If we have an attached source, then navitage to that tab
    // and the line of the PC on that tab
    if (pSrc)
    {
        BringPCOnWindow();

        // Make this tab the focus
        pSrc->pTab->SetFocus();
        m_pParent->DrawSourceWindow();
    }
    return 0;
}

/*
==============================================================================
Reset the LISA core
==============================================================================
*/
int CLisa::Reset(int argc, char* argv[])
{
    char  *cmd_argv[2] = { (char *) "halt", (char *) "1"};

    if (!m_Connected)
        return 1;

    // Halt the processor
    Halt(1, cmd_argv);

    WriteUartString("t");

    m_pParent->DrawSourceWindow();
    return 0;
}

/*
==============================================================================
Delete breakpoints
==============================================================================
*/
int CLisa::Delete(int argc, char* argv[])
{
    int         x;

    if (!m_Connected)
        return 1;

    if (argc == 1)
    {
        // Delete all breakpoints
        for (x = 0; x < 4; x++)
        {
            WriteReg(8 + x, 0);
            m_Breakpoints[x] = 0;
            usleep(2000);
        }
    }
    else
    {
        x = atoi(argv[1]);
        if (x >= 0 && x < 4)
        {
            WriteReg(8 + x, 0);
            m_Breakpoints[x] = 0;
        }
    }
    m_pParent->DrawSourceWindow();

    return 0;
}

/*
==============================================================================
Set or show breakpoints
==============================================================================
*/
int CLisa::Break(int argc, char* argv[])
{
    int         x;
    uint32_t    regVal;

    if (!m_Connected)
        return 1;

    if (argc == 1)
    {
        // List current breakpoints
        for (x = 0; x < 4; x++)
        {
            if (ReadReg(8 + x, regVal) != -1)
            {
                m_Breakpoints[x] = regVal;
                // Test if breakpoint enabled
                if (regVal & 0x8000)
                {
                    const lisa::CdbLine *l = m_Cdb.loaded() ? m_Cdb.line_at(regVal & 0x7fff) : NULL;
                    if (l)
                        Printf("%d: 0x%04x  %s:%d in %s()", x, (uint16_t) (regVal & 0x7fff), l->file.c_str(), l->line,
                               m_Cdb.function_at(regVal & 0x7fff)->name.c_str());
                    else
                        Printf("%d: 0x%04x", x, (uint16_t) (regVal & 0x7fff));
                }
            }
        }
    }
    else
    {
        // file:line, a line, a function, *addr (an address when there is no .cdb)
        uint16_t    addr;
        std::string what;

        if (!ResolveLocation(argv[1], addr, what))
            return 1;
        // not right after a store: on TT07 a breakpoint halt there loses
        // the store and overwrites RAM[IX] (Cdb::is_store)
        uint16_t safe = SafeStop(addr);
        if (safe != addr)
        {
            Printf("(0x%04x follows a store, which a TT07 breakpoint would lose: planted at 0x%04x, reported as 0x%04x)",
                   addr, safe, addr);
            addr = safe;
        }

        // Try to find an empty breakpoint
        for (x = 0; x < 4; x++)
        {
            if (ReadReg(8 + x, regVal) != -1)
            {
                // Test if breakpoint enabled
                if ((regVal & 0x8000) == 0)
                {
                    regVal = addr | 0x8000;
                    WriteReg(8 + x, regVal);
                    m_Breakpoints[x] = regVal;
                    Printf("breakpoint %d at 0x%04x%s%s", x, addr, what.empty() ? "" : ": ", what.c_str());
                    break;
                }
            }
        }

        if (x == 4)
            Printf("All 4 breakpoints already used");
        m_pParent->DrawSourceWindow();
    }
    // (this used to write 0 to the status register, which resumes a halted
    // core: setting a breakpoint must not start the program)
    return 0;
}

/*
==============================================================================
Attach the current file to the running app
==============================================================================
*/
int CLisa::Attach(int argc, char* argv[])
{
    CTab        *pTab;
    lisa_src_t  *pSrc;
    int         shuttle;

    pTab = m_pTui->GetActiveSrcTab();
    if (pTab == NULL)
    {
        Printf("Please open source to attach first");
        return 1;
    }

    // Find the source associated with the active tab
    pSrc = m_pSrcs;

    // Scan all sources
    while (pSrc)
    {
        if (pSrc->pTab == pTab)
            break;
        pSrc = pSrc->pNext;
    }

    // Test if source found
    if (pSrc == NULL)
        return 1;

    // Test if current tab source is OBJ
    if (pSrc->type != LISA_SRC_TYPE_OBJ && pSrc->type != LISA_SRC_TYPE_LIST)
    {
        Printf("Current tab is not an object file");
        return 1;
    }

    // Mark current tab as 'attached'
    pSrc->attached = 1;
    m_pAttachedSrc = pSrc;
    if (argc > 1)
    {
        shuttle = atoi(argv[1]);
        if (shuttle != 6 && shuttle != 7)
            Printf("Unknown shuttle %d", shuttle);
        else
            m_Shuttle = shuttle;
    }
    Printf("Attached current tab as active program");

    m_pParent->DrawSourceWindow();

    return 0;
}

/*
==============================================================================
Perform single step
==============================================================================
*/
void CLisa::SingleStep(void)
{
    char  *cmd_argv[2] = { (char *) "step", (char *) "1"};

    if (!m_Connected)
        return;

    if (m_pAttachedSrc == NULL)
    {
        Printf("Please attach or load an object file first");
        return;
    }

    Step(2, cmd_argv);
}

/*
==============================================================================
Single step the LISA core
==============================================================================
*/
int CLisa::Step(int argc, char* argv[])
{
    int         steps = 1;

    if (!m_Connected)
        return 1;

    if (argc > 1)
        steps = atoi(argv[1]);
    if (steps < 1)
        steps = 1;

    // By breakpoints on the addresses the instruction can continue at (the
    // TT07 step bit pulses too briefly to execute anything), a store run on
    // with what follows it (StepInsn, LisaCdb.cxx)
    for (int x = 0; x < steps; x++)
    {
        if (gLastWasCtrlC)
            break;
        if (StepInsn() < 0)
            break;
    }

    // Bring PC on the tab
    if (m_pAttachedSrc)
        BringPCOnWindow();
    ShowLocation(false);
    return 0;
}

/*
==============================================================================
Read and show LISA SP
==============================================================================
*/
int CLisa::SP(int argc, char* argv[])
{
    uint32_t   sp;

    if (!m_Connected)
        return 1;

    if (ReadReg(REG_SP, sp) == -1)
        Printf("Error reading LISA");
    else
        Printf("0x%04X", sp);
    return 0;
}

/*
==============================================================================
Read and show LISA PC
==============================================================================
*/
int CLisa::PC(int argc, char* argv[])
{
    uint32_t   pc;

    if (!m_Connected)
        return 1;

    if (ReadReg(REG_PC, pc) == -1)
        Printf("Error reading LISA");
    else
        Printf("0x%04X", pc);
    return 0;
}

/*
==============================================================================
Read a LISA register
==============================================================================
*/
int CLisa::Read(int argc, char *argv[])
{
    uint32_t   val;
    int        reg;

    if (!m_Connected)
        return 1;

    reg = strtol(argv[1], NULL, 0);
    if (ReadReg(reg, val) == -1)
        Printf("Error reading LISA");
    else
        Printf("0x%04X", val);
    return 0;
}

/*
==============================================================================
Write a LISA register
==============================================================================
*/
int CLisa::Write(int argc, char *argv[])
{
    uint32_t   val;
    int        reg;

    if (!m_Connected)
        return 1;

    reg = strtol(argv[1], NULL, 0);
    val = strtol(argv[2], NULL, 0);
    if (WriteReg(reg, val) == -1)
        Printf("Error writing LISA");
    return 0;
}

/*
==============================================================================
help [command]: the command table's usage and description, plus the longer
explanation of the commands that need one
==============================================================================
*/
static const struct { const char *name; const char *text; } gHelpDetail[] =
{
    {"connect", "connect <port> [baud]\n"
                "  Opens the demo board's USB serial port (e.g. /dev/cu.usbmodem1101; only a\n"
                "  serial device is accepted). From the MicroPython REPL it selects tt_um_lisa,\n"
                "  starts the uartPass pass-through, finds LISA's debugger (sending +++ if a\n"
                "  program had the UART), then runs `setup`. Close the LISA Commander or any\n"
                "  mpremote session first: only one program can hold the port."},
    {"setup",   "setup [apply | defaults]\n"
                "  Without an argument opens the SETUP tab, a form over the debugger's\n"
                "  configuration registers (read from the chip when connected):\n"
                "    program fetch (LISA1), data cache (LISA2), TTLC: chip select and base\n"
                "    debugger flash port: chip select\n"
                "    per chip select: flash/RAM, SPI/QSPI, 24/16-bit addresses, dummy cycles\n"
                "    SPI mode, SCLK divider, CE delay; data cache on/off and map; pin muxes\n"
                "  Up/Down/Tab move, Left/Right/Space change, hex digits edit, Enter on a\n"
                "  button, Esc cancels. OK checks the settings, writes the registers, reads\n"
                "  each back and keeps them: connect applies them from then on (saved in\n"
                "  .tui_prefs). `setup apply` writes the kept settings again (after a\n"
                "  project reset); `setup defaults` goes back to the defaults."},
    {"debug",   "debug <prog.ihx>\n"
                "  Loads the program image and the .cdb that `sdcc -mlisa --debug` writes\n"
                "  beside it, opens each C source in a tab (-> marks the PC's line, * a\n"
                "  breakpoint's) and warns if the flash does not hold this image. It does not\n"
                "  program the flash."},
    {"break",   "break [<file:line> | <line> | <function> | *<addr>]\n"
                "  Without an argument lists the 4 hardware breakpoints. A bare line is in the\n"
                "  active tab's file; a function stops at its first line. An address right\n"
                "  after a store is moved past it (on TT07 a halt there loses the store and\n"
                "  overwrites RAM[IX]); the stop is still reported as the line you gave."},
    {"run",     "run\n"
                "  Runs until a breakpoint, a brk or Ctrl-C (like gdb's continue). The program\n"
                "  has the UART meanwhile; what it prints appears as [LISA] ... lines."},
    {"next",    "next\n"
                "  Runs to the next source line, over calls. Breakpoints go on the line's\n"
                "  exits (computed from the image) rather than stepping, so it is quick."},
    {"into",    "into\n"
                "  Like next, but stops at the first line of a called function."},
    {"finish",  "finish\n"
                "  Runs until the current function returns, then on to the caller's next line."},
    {"print",   "print <expr>\n"
                "  A variable in the current function or a global: print s, print arr[2],\n"
                "  print p->x, print *p, print &g, print st.m. Peripheral registers cannot be\n"
                "  read through the debugger."},
    {"halt",    "halt\n"
                "  Stops a running core by putting breakpoints on the loop it is running\n"
                "  (sampled PC), so no store is lost; only if that fails, an immediate halt\n"
                "  with a warning. Ctrl-C during run/next does the same."},
    {"step",    "step [n]\n"
                "  One machine instruction (n times). A store is run on with what follows it."},
    {"read",    "read <reg>\n"
                "  A debugger register: 0 status, 1 flags/A, 2 PC, 3 SP, 4 RA, 5 IX,\n"
                "  6 data at IX, 7 data address, 8-11 breakpoints, 0x10+ configuration."},
    {"set",     "set <reg>=<value>  (a, pc, sp, ra, ix)"},
};

int CLisa::Help(int argc, char *argv[])
{
    if (argc > 1)
    {
        for (const auto& d : gHelpDetail)
            if (strcasecmp(d.name, argv[1]) == 0)
            {
                // one line at a time: Printf takes a line
                std::string t = d.text;
                size_t start = 0;
                while (start <= t.size())
                {
                    size_t nl = t.find('\n', start);
                    Printf("%s", t.substr(start, nl == std::string::npos ? std::string::npos : nl - start).c_str());
                    if (nl == std::string::npos)
                        break;
                    start = nl + 1;
                }
                return 0;
            }
        for (int x = 0; m_TuiCmds[x].name; x++)
            if (strcasecmp(m_TuiCmds[x].name, argv[1]) == 0)
            {
                Printf("%s %s", m_TuiCmds[x].name, m_TuiCmds[x].usage);
                Printf("  %s", m_TuiCmds[x].help);
                return 0;
            }
        Printf("No command %s (help lists them)", argv[1]);
        return 1;
    }
    Printf("Commands (help <command> explains one; Ctrl-C interrupts run/next):");
    for (int x = 0; m_TuiCmds[x].name; x++)
    {
        const char *usage = m_TuiCmds[x].usage;
        // the table's usage sometimes repeats the name ("load <file>"): show the arguments only
        size_t n = strlen(m_TuiCmds[x].name);
        if (strncmp(usage, m_TuiCmds[x].name, n) == 0 && (usage[n] == ' ' || usage[n] == 0))
            usage += usage[n] ? n + 1 : n;
        Printf("  %-8s %-16s %s", m_TuiCmds[x].name, usage, m_TuiCmds[x].help);
    }
    return 0;
}

/*
==============================================================================
Set a core register by name: set pc=0x100, set sp 0x7f
==============================================================================
*/
int CLisa::Set(int argc, char *argv[])
{
    static const struct { const char *name; int reg; } regs[] =
        { {"a", REG_ACC}, {"acc", REG_ACC}, {"pc", REG_PC}, {"sp", REG_SP}, {"ra", REG_RA}, {"ix", REG_IX} };
    char        name[16];
    const char *pVal;
    uint32_t    val;
    int         x;

    if (!m_Connected)
        return 1;

    // "reg=val" in one argument, or "reg val"
    pVal = strchr(argv[1], '=');
    if (pVal)
    {
        snprintf(name, sizeof(name), "%.*s", (int) (pVal - argv[1]), argv[1]);
        pVal++;
    }
    else if (argc > 2)
    {
        snprintf(name, sizeof(name), "%s", argv[1]);
        pVal = argv[2];
    }
    else
    {
        Printf("Usage: set reg=val   (a, pc, sp, ra, ix)");
        return 1;
    }

    for (x = 0; x < (int) (sizeof(regs) / sizeof(regs[0])); x++)
        if (strcasecmp(regs[x].name, name) == 0)
            break;
    if (x == (int) (sizeof(regs) / sizeof(regs[0])))
    {
        Printf("Unknown register %s (a, pc, sp, ra, ix)", name);
        return 1;
    }

    val = strtoul(pVal, NULL, 0);
    if (WriteReg(regs[x].reg, val) == -1)
        Printf("Error writing LISA");
    else
        m_pParent->DrawSourceWindow();
    return 0;
}

/*
==============================================================================
Show directory listing
==============================================================================
*/
int CLisa::Ls(int argc, char* argv[])
{
    FILE    *fp;
    char    line[1024];
    int     x;

    // Build the 'ls' line
    strcpy(line, argv[0]);
    for (x = 1; x < argc; x++)
    {
        strcat(line, " ");
        strcat(line, argv[x]);
    }

    /* Pipe stderr to stdout */
    strcat(line, " 2>&1");
    fp = popen(line, "r");
    if (fp == NULL)
    {
        Printf("error: unable to execute command\n");
        return -1;
    }
    while (fgets(line, sizeof(line), fp))
        Printf("%s", line);

    return pclose(fp);
}

/*
==============================================================================
Enable LISA terminal
==============================================================================
*/
int CLisa::Term(int argc, char* argv[])
{
    CTab         *pTab;
    lisa_src_t   *pSrc;
    char         buffer[8];

    // Ensure we are connected
    if (!m_Connected)
    {
        Printf("Please connect to the target first");
        return OK;
    }

    // Test if this source already loaded
    pSrc = m_pSrcs;
    while (pSrc)
    {
        // Test if this source name matches
        if (strcmp(pSrc->name, "TERM") == 0)
        {
            // Just make the TERM tab active
            pSrc->pTab->SetFocus();

            break;
        }
        
        // Point to next source in the list
        pSrc = pSrc->pNext;
    }
  
    // Allocate a source structure
    if (pSrc == NULL)
    {
        pSrc = (lisa_src_t *) malloc(sizeof(lisa_src_t));
        if (pSrc == NULL)
            return -1;
        
        pSrc->sourceLineCount = 0;
        pSrc->fd              = NULL;
        pSrc->topLine         = 1;
        pSrc->pSymbolList     = NULL;
        pSrc->pFirstDynLine   = NULL;
        pSrc->pProgram        = NULL;
        pSrc->pListCols       = NULL;
        pSrc->pNext           = NULL;
        pSrc->pTab            = NULL;
        pSrc->headerLineno    = 0;
        pSrc->colCount        = 0;
        pSrc->type            = LISA_SRC_TYPE_TERM;
        strcpy(pSrc->name, "TERM");
    }
  
    // Create a new tab for this source
    if (pSrc->pTab == NULL)
    {
        pTab = m_pParent->CreateNewTab("TERM");
        
        if (pTab != NULL)
        {
            // Add this source to our linked list of sources
            pSrc->pNext = m_pSrcs;
            m_pSrcs = pSrc;
            
            // Attach the source to the tab
            pTab->AttachTuiSource(this, (void *) pSrc);
            pTab->SourceFirstLine(pSrc->topLine);
            pSrc->pTab = pTab;
        }
    }
    
    // Draw / Redraw the tab
    pSrc->pTab->SetFocus();
    m_pTui->FocusTabs();
//    m_pTui->SetSourceFocus();

    m_pTermSrc = pSrc;

    // Enable LISA terminal mode
    if (!m_LisaHasUart)
    {
        // Ensure the watch is paused
        m_pTui->m_WatchPaused = 1;
        usleep(300000);

        // Send 'l'isa UART command
        strcpy(buffer, "l");
        WriteUartString(buffer);
    }

    return OK;
}

/*
==============================================================================
Await a substring.
==============================================================================
*/
int CLisa::AwaitString(const char *subResp, int retry, int sleepms)
{
    int     x;
    char    buffer[512];

    for (x = 0; x < retry; x++)
    {
        buffer[0] = 0;
        ReadUartString(m_pSer, buffer, sizeof(buffer));
        if (strstr(buffer, subResp) != NULL)
        {
            m_Response = buffer;
            return 1;
        }
        usleep(sleepms * 1000);
    }

    return 0;
}

/*
==============================================================================
Perform a "send command", "wait for sub response".  Waits until subResp
substring received.
==============================================================================
*/
int CLisa::CmdResponse(const char *cmd, const char *subResp, int retry, int sleepms)
{
    char    buffer[512];

    snprintf(buffer, sizeof(buffer), "%s\r", cmd);

    WriteUartString(buffer);
    usleep(200000);

    return AwaitString(subResp, retry, sleepms);
}

/*
==============================================================================
Handle the 'connect' command.  This connects to the LISA UART and interrogates
the current state.  LISA is served via RP2040 console UART (typically but not
required) and so we may get a Micropython REPL '>>>'.  We need to report 
this (or auto init the connection if requested instead of reporting).
==============================================================================
*/
int CLisa::Connect(int argc, char* argv[])
{
    char buffer[256];
    bool repl = false;
    bool connected = false;
    int  baudRate = 115200;
    int  ret;

    if (argc < 2) {
        Printf("Usage: connect <port> [baud]");
        return 1;
    }

    // Only a serial device: the port is opened read-write and the debugger
    // traffic would otherwise be written into whatever file was named
    // (a test image was damaged that way)
    struct stat st;
    if (stat(argv[1], &st) != 0 || !S_ISCHR(st.st_mode)) {
        Printf("%s is not a serial device", argv[1]);
        return 1;
    }
    if (m_PortOpen) {
        ser_deinit(m_pSer);
        m_PortOpen = 0;
        m_Connected = 0;
    }

    // Try to open the specified port
    if (ser_init(argv[1], &m_pSer) != SER_NO_ERROR) {
        Printf("Connection failed");
        return 1;
    }

    // Test if baud rate given
    if (argc == 3)
    {
        baudRate = atoi(argv[2]);
        if (baudRate < 0)
            baudRate = 115200;
    }

    // Set the baud rate
    //ser_set_baud(m_pSer, 115200);
    ser_set_baud(m_pSer, baudRate);
    m_PortOpen = 1;

    if (CmdResponse("", ">>>", 5, 200))
    {
        Printf("Micropython REPL detected");
        repl = true;
    }

    if (repl)
    {
        if (CmdResponse("print(tt.shuttle)", "Shuttle", 5, 200))
        {
            char    shuttle[64];

            if (m_Response.find("tt06") != std::string::npos)
            {
                Printf("Shuttle tt06 detected\n");
                strcpy(shuttle, "tt.shuttle.tt_um_lisa.enable()");
                m_Shuttle = 6;
            }
            else if (m_Response.find("tt07") != std::string::npos)
            {
                Printf("Shuttle tt07 detected\n");
                strcpy(shuttle, "tt.shuttle.tt_um_lisa.enable()");
                m_Shuttle = 7;
            }

            CmdResponse(shuttle, "Clocking", 10, 1000);
            AwaitString(">>>", 5, 100);
            usleep(20000);
            if (CmdResponse("import uartPass", ">>>", 2, 50))
            {
                WriteUartString("uartPass.passthrough()\r");
                usleep(200000);
            }
        }

        Printf("REPL init complete, entering debugger passthrough...");
        usleep(200000);
    }

    // Now get the attention of the LISA debugger
    WriteUartString("\n");
    usleep(20000);
    WriteUartString("\n");
    usleep(20000);
    WriteUartString("v");
    if (!AwaitString("lisav1", 6, 50))
    {
        Printf("Trying Hayes escape sequence (+++)...");
        WriteUartString("+++");
        usleep(1000000);  // wait for state change
        ReadUartString(m_pSer, buffer, sizeof(buffer));
    }

    ReadUartString(m_pSer, buffer, sizeof(buffer));

    // Now test with a read command
    uint32_t val;
    ret = ReadReg(0, val);

    if (ret != -1 && (val & 0xFF00) == 0xFF00)
    {
        Printf("LISA debugger active (%04x response confirmed)", val);
        connected = true;
    }
    else
        Printf("Failed to recover LISA debugger");

    if (!repl && !connected)
        Printf("Connection completed, but UART is in unknown state");
    else
        Printf("Connected and ready");

    m_Connected = 1;
    if (connected)
        SetupDebugger(0);

    ReadBreakpoints();
    return 0;
}

/*
================================================================================
Read the LISA breakpoints
================================================================================
*/
void CLisa::ReadBreakpoints(void)
{
    int         x;
    uint32_t    regVal;

    // Try to find an empty breakpoint
    for (x = 0; x < 4; x++)
    {
        if (ReadReg(8 + x, regVal) != -1)
            m_Breakpoints[x] = regVal;
    }
}

/*
================================================================================
Open the program from the specified list file
================================================================================
*/
int CLisa::OpenFromListFile(lisa_src_t *pSrc, char *line, int maxlen)
{
    FILE        *fd;
    int         pc;
    int         opcode;
    int         lineno;
    int         isList = 1;
    char        *pTok;

    // Initialize the PC to zero
    lineno      = 0;
    pSrc->sourceLineCount = 0;
    pSrc->lineStarts[0] = 0;

    // Allocate a program structure
    pSrc->pProgram = (lisa_program_t *) malloc(sizeof(lisa_program_t));
    if (pSrc->pProgram == NULL)
    {
      Printf("OUT OF MEMORY allocating program structure!");
      return -1;
    }

    // Clear out the program
    memset(pSrc->pProgram, 0, sizeof(lisa_program_t));
    
    // Get the source fd
    fd = pSrc->fd;

    // Parse through the file
    while (fgets(line, maxlen, fd) != NULL)
    {
        // Increment the lineno
        lineno++;

        // Increment source line count
        pSrc->sourceLineCount++;
        pSrc->lineStarts[pSrc->sourceLineCount] = ftell(fd);

        // Test for blank line
        if (line[0] == '\n')
            continue;
        
        // Test for valid PC/address entry
        if (strncmp(line, "0x", 2) != 0 && strncmp(line, "        0x", 10) != 0 &&
            strncmp(line, "              ", 14) != 0)
        {
            isList = 0;
            continue;
        }
        
        // Skip non-opcode lines
        if (strncmp(line, "              ", 14) == 0)
            continue;
        
        // Tokenize the line to get address and opcode
        pTok = strtok(line, " ");
        if (pTok != NULL)
        {
            if (line[0] == ' ')
                pc++;
            else
            {
                // Get the address
                pc = strtol(pTok, NULL, 16);

                // Parse the next token and test for opcode format
                pTok = strtok(NULL, " ");
            }

            // Get the opcode
            if (strncmp(pTok, "0x", 2) != 0)
            {
                isList = 0;
                continue;
            }
            opcode = strtol(pTok, NULL, 16);
         
            // Tracking for single-step / debugging
            pSrc->pProgram->m_LstLine[pc] = lineno;

            // Update the program
            pSrc->pProgram->m_Program[pc] = opcode;
            pSrc->lineAddrs[lineno-1] = pc;

            // Calculate program size
            if (pc > pSrc->pProgram->m_Size)
                pSrc->pProgram->m_Size = pc;
        }
    }

    // Now set initial PC and lineno in edit windows
    return isList;
}

/*
==============================================================================
Determine if file is object type
==============================================================================
*/
int CLisa::IsObjectFile(lisa_src_t *pSrc)
{
    char    line[128];
    bool    isObject = true;
    int     len;
    FILE    *fd;
    int     offset = 0;
    int     lineNo = 0;
    int     address = 0;

    if ((fd = fopen(pSrc->path, "r")) == NULL)
        return 0;

    pSrc->lineStarts[0] = 0;
    while (fgets(line, sizeof(line), fd) != NULL)
    {
        // Trim end of string
        len = strlen(line);
        pSrc->lineStarts[lineNo] = offset;
        pSrc->lineAddrs[lineNo++] = address++;

        // Update offset of next line
        offset += len;
        while (len && (line[len-1] == ' ' || line[len-1] == '\n' ||
               line[len-1] == '\r'))
        {
            line[--len] = 0;
        }

        if (len == 0)
            continue;

        // Test if line is more than 4 characters
        if (len != 4)
        {
            isObject = false;
            break;
        }

        // Test if each character is hex digit
        for (len = 0; len < (int) strlen(line); len++)
        {
            if (!isxdigit(line[len]))
            {
                isObject = false;
                break;
            }
        }

        // Test for ldx opcode
        if (strcmp(line, "A180") == 0 || strcmp(line, "a180") == 0)
        {
            // Read next line from file so lineStarts for next line is correct
            if (fgets(line, sizeof(line), fd) != NULL)
            {
                len = strlen(line);
                offset += len;
                address++;
            }
        }
    }

    if (isObject)
    {
        Printf("Opening as object file");
        pSrc->fd = fd;
        fseek(fd, 0, SEEK_SET);
        pSrc->sourceLineCount = lineNo;
    }
    else
        fclose(fd);
    return isObject;
}

/*
==============================================================================
Parse the m_Filename file
==============================================================================
*/
int CLisa::ParseFile(lisa_src_t *pSrc, char *filename)
{
    char            line[512];
    int             offset;
    int             fd;
    int             x;
    int             len;
    bool            in_symbol;
    int             col;
    char            label[128];
    unsigned int    label_len;
    int             ret;
    lisa_symbol_t *pSymbol = NULL;
  
    // Try to open the file
    for (x = 0; m_Paths[x] != NULL && x < LISA_MAX_PATHS; x++)
    {
        snprintf(line, sizeof(line), "%s/%s", m_Paths[x], filename);
        if ((fd = open(line, O_RDONLY)) != -1)
        {
            break;
        }
    }
  
    // Test if the file was opened
    if (fd == -1)
    {
        // Not found.  Try prepending the working directory and then open
        if (filename[0] != '/' && filename[0] != '~')
            snprintf(line, sizeof(line), "%s/%s", m_WorkingDir, filename);
        else
            snprintf(line, sizeof(line), "%s", filename);
        if ((fd = open(line, O_RDONLY)) == -1)
        {
            Printf("Unable to open file %s: %s", line, strerror(errno));
            return -1;
        }
    }
    strcpy(pSrc->path, line);
    strncpy(pSrc->filename, filename, sizeof(pSrc->filename) - 1);
    pSrc->filename[sizeof(pSrc->filename) - 1] = 0;
    len = strlen(filename);
    if (len > 4 && strcmp(&filename[len-4], ".lst") == 0)
    {
        // Open the new file for use
        close(fd);
        if ((pSrc->fd = fopen(pSrc->path, "r")) == NULL)
        {
            Printf("Unable to open file");
            return -1;
        }
        ret = OpenFromListFile(pSrc, line, sizeof(line));
        if (ret)
        {
            Printf("Program size: %d opcodes", pSrc->pProgram->m_Size);
            pSrc->type = LISA_SRC_TYPE_LIST;
        }
        else
            pSrc->type = LISA_SRC_TYPE_OTHER;
        return OK;
    }
  
    // Test if file looks like object file
    if (IsObjectFile(pSrc))
    {
        close(fd);
        pSrc->type = LISA_SRC_TYPE_OBJ;
        return 0;
    }
  
    // Initialize control variables
    pSrc->sourceLineCount = 0;
    pSrc->lineStarts[0] = 0;
    offset = 0;
    in_symbol = false;
    col = 0;
    label[0] = '\0';
    label_len = 0;
  
    // Read 512 bytes at a time from the file
    while ((len = read(fd, line, sizeof(line))) != 0)
    {
        // Loop for each byte received
        for (x = 0; x < len; x++)
        {
            if (line[x] == '\n')
            {
                // Increment source line count
                pSrc->sourceLineCount++;
                pSrc->lineStarts[pSrc->sourceLineCount] = offset + x + 1;
                col = 0;
            }
            
            // Test for new symbol start
            else if (col == 0)
            {
                // Test for symbol start
                col++;
                if (line[x] != '/' && line[x] != ' ' && line[x] != 9 &&
                    line[x] != '#')
                {
                    // Start new symbol
                    label[0] = line[x];
                    label[1] = '\0';
                    label_len = 1;
                    in_symbol = true;
                }
            }
            else if (in_symbol)
            {
                // Increment column
                col++;
                
                // Test for end of symbol
                if ((line[x] == ':' && line[x+1] != ':' && line[x-1] != ':') || line[x] == ' ')
                {
                    // Ensure m_Name is not 'set'
                    if (!(strcmp(label, "set") == 0 || strcmp(label, "rev") == 0))
                    {
                        // Create a new symbol
                        pSymbol = (lisa_symbol_t *) malloc(label_len + 1 + sizeof(*pSymbol));
                        pSymbol->name = ((char *) pSymbol) + sizeof(*pSymbol);
                        strcpy(pSymbol->name, label);
                        pSymbol->line = pSrc->sourceLineCount;
                      
                        // Add symbol to the symbol list
                        pSymbol->pNext = pSrc->pSymbolList;
                        pSrc->pSymbolList = pSymbol;
                      
                        // Clear out for next symbol
                        pSymbol = NULL;
                        label[0] = '\0';
                        label_len = 0;
                    }
                    
                    in_symbol = false;
                }
                else if (label_len < sizeof(label) - 1)
                {
                    // Append character to label
                    label[label_len++] = line[x];
                    label[label_len] = '\0';
                }
            } 
            else
            {
                // Increment column
                col++;
            }
        }
        offset += len;
    }
    close(fd);
  
    // Open the new file for use
    if ((pSrc->fd = fopen(pSrc->path, "r")) == NULL)
    {
        Printf("Unable to open file");
        return -1;
    }
  
    return OK;
}

/*
==============================================================================
Open the specified file
==============================================================================
*/
int CLisa::Open(int argc, char *argv[])
{
    int          ret;
    CTab         *pTab;
    lisa_src_t   *pSrc;
  
    // Test if this source already loaded
    pSrc = m_pSrcs;
    while (pSrc)
    {
        // Test if this source name matches
        if (strcmp(pSrc->name, argv[1]) == 0)
        {
            // Configure pSrc for reload
            if (pSrc->pSymbolList != NULL)
                free(pSrc->pSymbolList);
            if (pSrc->pProgram != NULL)
                free(pSrc->pProgram);
            if (pSrc->pListCols != NULL)
                free(pSrc->pListCols);
            if (pSrc->fd != NULL)
            {
                // Close the file so it can be reopened
                fclose(pSrc->fd);
                pSrc->fd = NULL;
            }
            
            pSrc->sourceLineCount = 0;
            pSrc->pSymbolList     = NULL;
            pSrc->pFirstDynLine   = NULL;
            pSrc->pProgram        = NULL;
            pSrc->pListCols       = NULL;
            pSrc->headerLineno    = 0;
            break;
        }
        
        // Point to next source in the list
        pSrc = pSrc->pNext;
    }
  
    // Allocate a source structure
    if (pSrc == NULL)
    {
        pSrc = (lisa_src_t *) malloc(sizeof(lisa_src_t));
        if (pSrc == NULL)
            return -1;
        
        pSrc->sourceLineCount = 0;
        pSrc->fd              = NULL;
        pSrc->topLine         = 1;
        pSrc->pSymbolList     = NULL;
        pSrc->pFirstDynLine   = NULL;
        pSrc->pProgram        = NULL;
        pSrc->pListCols       = NULL;
        pSrc->pNext           = NULL;
        pSrc->pTab            = NULL;
        pSrc->headerLineno    = 0;
        pSrc->colCount        = 0;
        pSrc->attached        = 0;
        pSrc->type            = LISA_SRC_TYPE_ASM;
        pSrc->cdb_file[0]     = 0;
        strcpy(pSrc->name, argv[1]);
    }
  
    // Parse the file to check if it is a listing, etc.
    ret = ParseFile(pSrc, argv[1]);
    if (ret == OK)
    {
        // Not sure m_Filename is ever used, but populate it in case
        strcpy(m_Filename, argv[1]);
        
        // Create a new tab for this source
        if (pSrc->pTab == NULL)
        {
            int pos = strlen(m_Filename);
            while (pos && m_Filename[pos-1] != '/')
                pos--;
            if (pos)
                pTab = m_pParent->CreateNewTab(&m_Filename[pos]);
            else
                pTab = m_pParent->CreateNewTab(m_Filename);
            
            if (pTab != NULL)
            {
                // Add this source to our linked list of sources
                pSrc->pNext = m_pSrcs;
                m_pSrcs = pSrc;
                
                // Attach the source to the tab
                pTab->AttachTuiSource(this, (void *) pSrc);
                pTab->SourceFirstLine(pSrc->topLine);
                pSrc->pTab = pTab;
            }
        }
        
        // Draw / Redraw the tab
        pSrc->pTab->SetFocus();
        m_pParent->DrawSourceWindow();
    }
    else
    {
        /* Error Parsing the file.  Free the memory */
        free(pSrc);
    }
  
    return ret;
}

/*
==============================================================================
Disassemble an opcode
==============================================================================
*/
void CLisa::DisassembleOpcode(uint32_t opcode, char *pStr, int len)
{
    uint8_t opHi = (opcode >> 14) & 0x3; // Top 2 bits
    char buf[128] = {0};
    const char *sCond[] = { "EQ", "NE", "NC", "C", "GT", "LT", "GTE", "LTE"};

    switch (opHi)
    {
        case 0b00:
        {
            // jal
            uint16_t addr = opcode & 0x1FFF;
            snprintf(buf, sizeof(buf), "jal       0x%04x", addr);
            break;
        }

        case 0b10:
        {
            uint8_t subcode = (opcode >> 10) & 0x0F;
            uint8_t imm8 = opcode & 0xFF;

            switch (subcode)
            {
                // ldi
                case 0x0:
                    snprintf(buf, sizeof(buf), "ldi       %d", imm8);
                    if (imm8 >= ' ' && imm8 <= '~')
                        snprintf(&buf[strlen(buf)], sizeof(buf), "  '%c'", imm8);
                    break;

                // mulu
                case 0x01:
                {
                    int8_t imm7 = opcode & 0x7F;
                    bool use_sp = (opcode >> 7) & 0x1;
                    const char *reg = use_sp ? "sp" : "ix";
                    snprintf(buf, sizeof(buf), "mulu      %d(%s)", imm7, reg);
                    break;
                }

                case 0x2:
                {
                    uint8_t bits = (opcode >> 7) & 0x7;

                    switch (bits)
                    {
                        case 0x4:
                            snprintf(buf, sizeof(buf), "ret");
                            break;

                        case 0x6:
                            snprintf(buf, sizeof(buf), opcode & 0x40 ? "rets" : "rc");
                            break;

                        case 0x7:
                            snprintf(buf, sizeof(buf), "rz");
                            break;

                        case 0x5:
                        {
                            uint8_t tail = (opcode >> 2) & 0xFF;

                            switch (tail)
                            {
                                case 0xA0: snprintf(buf, sizeof(buf), "call_ix"); break;
                                case 0xA8: snprintf(buf, sizeof(buf), "jmp_ix"); break;
                                case 0xB0: snprintf(buf, sizeof(buf), "xchg_ra"); break;
                                case 0xB1: snprintf(buf, sizeof(buf), "xchg_ia"); break;
                                case 0xB2: snprintf(buf, sizeof(buf), "xchg_sp"); break;
                                case 0xB3: snprintf(buf, sizeof(buf), "spix"); break;
                                case 0xB4: snprintf(buf, sizeof(buf), "cpx       ra"); break;
                                case 0xB6: snprintf(buf, sizeof(buf), "cpx       sp"); break;
                                default:   snprintf(buf, sizeof(buf), "unknown"); break;
                            }

                            break;
                        }

                        default:
                            snprintf(buf, sizeof(buf), "unknown");
                            break;
                    }

                    break;
                }

                // reti
                case 0x3:
                    snprintf(buf, sizeof(buf), "reti      %-3d", imm8);
                    if (imm8 >= ' ' && imm8 <= '~')
                        sprintf(&buf[strlen(buf)], "  '%c'", imm8);
                    break;

                // adc
                case 0x04:
                    snprintf(buf, sizeof(buf), "adc       %d", opcode & 0xFF);
                    break;

                // ads (signed SP offset)
                case 0x05:
                    snprintf(buf, sizeof(buf), "ads       %d", static_cast<int8_t>(opcode & 0xFF));
                    break;

                // adx (signed IX offset)
                case 0x06: 
                    snprintf(buf, sizeof(buf), "adx       %d", static_cast<int8_t>(opcode & 0xFF));
                    break;

                // dcx
                case 0x07:
                {
                    int8_t imm7 = opcode & 0x7F;
                    bool use_sp = (opcode >> 7) & 0x1;
                    const char *reg = use_sp ? "sp" : "ix";
                    snprintf(buf, sizeof(buf), "dcx       %d(%s)", imm7, reg);
                    break;
                }

                case 0x08:
                {
                    uint8_t bits = (opcode >> 2) & 0xFF;

                    switch (bits)
                    {
                        case 0x00: snprintf(buf, sizeof(buf), "shl"); break;
                        case 0x01: snprintf(buf, sizeof(buf), "shr"); break;
                        case 0x1C: snprintf(buf, sizeof(buf), "nop"); break;
                        case 0x1D: snprintf(buf, sizeof(buf), "notz"); break;
                        case 0x1F: snprintf(buf, sizeof(buf), "brk"); break;
                        case 0x20: snprintf(buf, sizeof(buf), "push_a"); break;
                        case 0x30: snprintf(buf, sizeof(buf), "pop_a"); break;
                        case 0x50: snprintf(buf, sizeof(buf), "amode     %d", opcode & 3); break;
                        case 0x58: snprintf(buf, sizeof(buf), "sra"); break;
                        case 0x59: snprintf(buf, sizeof(buf), "lra"); break;
                        case 0x5A: snprintf(buf, sizeof(buf), "push_ix"); break;
                        case 0x5B: snprintf(buf, sizeof(buf), "pop_ix"); break;
                        case 0x5C: snprintf(buf, sizeof(buf), "lddiv"); break;
                        case 0x5E: snprintf(buf, sizeof(buf), "savec"); break;
                        case 0x5F: snprintf(buf, sizeof(buf), "restc"); break;
                        case 0x60: snprintf(buf, sizeof(buf), "ldx"); break;
                        case 0x69: snprintf(buf, sizeof(buf), "addaxu"); break;
                        case 0x6B: snprintf(buf, sizeof(buf), "subaxu"); break;

                        default:
                        {
                            switch (bits & 0xFC)
                            {
                                case 0x00: snprintf(buf, sizeof(buf), "ldc       %d", opcode & 1); break;
                                case 0x04: snprintf(buf, sizeof(buf), "%s", (opcode & 2) ? "txau" : "txa");
                                case 0x08: snprintf(buf, sizeof(buf), "shl16     %d", opcode & 3); break;
                                case 0x0C: snprintf(buf, sizeof(buf), "shr16     %d", opcode & 3); break;
                                case 0x10: snprintf(buf, sizeof(buf), "btst      %d", opcode & 7); break;
                                case 0x18: snprintf(buf, sizeof(buf), "ldz       %d", opcode & 1); break;
                                case 0x40: snprintf(buf, sizeof(buf), "%s", (opcode & 2) ? "taxu" : "tax");
                                case 0x68 ... 0x6c:
                                           snprintf(buf, sizeof(buf), "%sax%s", bits&2 ? "sub" : "add",
                                                    bits&4 ? " c" : "");
                                           break;
                                case 0x70 ... 0x77:
                                           snprintf(buf, sizeof(buf), "ldac      %s", sCond[opcode&7]);
                                           break;

                                case 0x78: snprintf(buf, sizeof(buf), "%s%c", opcode & 2 ? "taf" : "tfa",
                                                    opcode & 1 ? 'c' : ' ');
                                           break;

                                // Fops
                                case 0x79 ... 0x7e:
                                {
                                    uint8_t fop = bits & 0x07;
                                    uint8_t fx = opcode & 0x03;
                                    switch (fop)
                                    {
                                        case 0x01:
                                            snprintf(buf, sizeof(buf), "fmul      f(%d)", fx);
                                            break;
                                        case 0x02:
                                            snprintf(buf, sizeof(buf), "fadd      f(%d)", fx);
                                            break;
                                        case 0x03:
                                            snprintf(buf, sizeof(buf), "fneg      f(%d)", fx);
                                            break;
                                        case 0x04:
                                            snprintf(buf, sizeof(buf), "fswap     f(%d)", fx);
                                            break;
                                        case 0x05:
                                            snprintf(buf, sizeof(buf), "fcmp      f(%d)", fx);
                                            break;
                                        case 0x06:
                                            snprintf(buf, sizeof(buf), "fdiv      f(%d)", fx);
                                            break;
                                        default:
                                            snprintf(buf, sizeof(buf), "unknown");
                                            break;
                                    }
                                    break;
                                }

                                // IF, IFTT, IFTE
                                case 0x80 ... 0xBF:
                                {
                                    static const char* cond_unsigned[8] = {
                                        "eq", "ne", "nc", "c", "gt", "lt", "gte", "lte"
                                    };
                                    static const char* cond_signed[8] = {
                                        "eq", "ne", "nc", "c", "gts", "lts", "gtes", "ltes"
                                    };

                                    static const char* typeStr[3] = {
                                        "if  ", "iftt", "ifte"
                                    };

                                    uint8_t s_bit = (opcode >> 6) & 0x1;
                                    uint8_t tt = (opcode >> 3) & 0x3;
                                    uint8_t cond = opcode & 0x07;

                                    const char* condStr = s_bit ? cond_signed[cond] : cond_unsigned[cond];

                                    snprintf(buf, sizeof(buf), "%s      %s", typeStr[tt], condStr);
                                    break;
                                }

                                // div
                                case 0xC0:
                                {
                                    const char* suffix[4] = { "ii", "ic", "ci", "cc" };
                                    snprintf(buf, sizeof(buf), "div       %s", suffix[opcode & 3]);
                                    break;
                                }

                                // rem
                                case 0xC4:
                                {
                                    const char* suffix[4] = { "ii", "ic", "ci", "cc" };
                                    snprintf(buf, sizeof(buf), "rem       %s", suffix[opcode & 3]);
                                    break;
                                }

                                case 0xC8:
                                    switch (opcode & 3)
                                    {
                                        case 0: snprintf(buf, sizeof(buf), "itof"); break;
                                        case 1: snprintf(buf, sizeof(buf), "ftoi"); break;
                                        case 2: snprintf(buf, sizeof(buf), "fclr"); break;
                                        default: snprintf(buf, sizeof(buf), "unknown"); break;
                                    }
                                    break;

                                default:
                                    break;
                            }
                            break;
                        }
                        break;
                    }

                    break;
                }

                // cpi
                case 0x09: 
                {
                    int8_t  imm8 = static_cast<int8_t>(opcode & 0xFF);
                    snprintf(buf, sizeof(buf), "cpi       %d", imm8);
                    if (imm8 >= ' ' && imm8 <= '~')
                        snprintf(&buf[strlen(buf)], sizeof(buf), "  '%c'", imm8);
                    break;
                }

                // Relative branches
                case 0xa ... 0xf: // 0b10 101b bbbbbbbb  => bz, br, bnz
                {
                    uint8_t top2 = (subcode >> 1) & 0x3;
                    int16_t offset = opcode & 0x7FF;
                    if (offset & 0x400)
                        offset |= 0xF800;

                    const char *mnem = (top2 == 0b01) ? "bnz":
                                       (top2 == 0b10) ? "br" :
                                       (top2 == 0b11) ? "bz" : "???";

                    snprintf(buf, sizeof(buf), "%-5s     %d", mnem, offset);
                    break;
                }

                default:
                    snprintf(buf, sizeof(buf), "unknown");
                    break;
            }

            break;
        }

        case 0b11:
        {
            uint8_t mainOp = (opcode >> 10) & 0xF;
            uint16_t imm9 = opcode & 0x1FF;
            bool use_sp = (opcode >> 9) & 0x1;
            const char *reg = use_sp ? "sp" : "ix";

            switch (mainOp)
            {
                case 0x00: snprintf(buf, sizeof(buf), "add       %d(%s)", imm9, reg); break;
                case 0x01: snprintf(buf, sizeof(buf), "mul       %d(%s)", imm9, reg); break;
                case 0x02: snprintf(buf, sizeof(buf), "sub       %d(%s)", imm9, reg); break;

                case 0x03:
                {
                    bool isStore = (opcode >> 9) & 0x1;
                    if (isStore)
                        snprintf(buf, sizeof(buf), "stxx      %d", imm9);
                    else
                        snprintf(buf, sizeof(buf), "ldxx      %d", imm9);
                    break;
                }

                case 0x04: snprintf(buf, sizeof(buf), "and       %d(%s)", imm9, reg); break;
                case 0x05: snprintf(buf, sizeof(buf), "andi      %d", opcode & 0xFF); break;
                case 0x06: snprintf(buf, sizeof(buf), "or        %d(%s)", imm9, reg); break;
                case 0x07: snprintf(buf, sizeof(buf), "swapi     %d", imm9); break;
                case 0x08: snprintf(buf, sizeof(buf), "xor       %d(%s)", imm9, reg); break;
                case 0x09: snprintf(buf, sizeof(buf), "inx       %d(%s)", imm9, reg); break;
                case 0x0A: snprintf(buf, sizeof(buf), "cmp       %d(%s)", imm9, reg); break;
                case 0x0B: snprintf(buf, sizeof(buf), "swap      %d(%s)", imm9, reg); break;
                case 0x0C: snprintf(buf, sizeof(buf), "ldax      %d(%s)", imm9, reg); break;
                case 0x0D: snprintf(buf, sizeof(buf), "lda       %d", imm9); break;
                case 0x0E: snprintf(buf, sizeof(buf), "stax      %d(%s)", imm9, reg); break;
                case 0x0F: snprintf(buf, sizeof(buf), "sta       %d", imm9); break;

                default:
                    snprintf(buf, sizeof(buf), "unknown");
                    break;
            }

            break;
        }

        default:
        {
            snprintf(buf, sizeof(buf), "unknown");
            break;
        }
    }

    strncpy(pStr, buf, len - 1);
    pStr[len - 1] = '\0';
}

/*
==============================================================================
Draw the lisa splashscreen
==============================================================================
*/
void CLisa::DrawSplash(WINDOW* pWnd)
{
    int sy = 3;
    int sx = 4;
    CTab    *pTab;

    pTab = m_pTui->GetFirstTab();
    if (pTab != NULL)
        return;
    
    wattron(pWnd, COLOR_PAIR(SYNTAX_PAIR_USER));
    mvwprintw(pWnd, sy++, sx,   " ");
    mvwprintw(pWnd, sy++, sx,   " ");
    mvwprintw(pWnd, sy++, sx,   "                            .{{{}}}}}}.");
    mvwprintw(pWnd, sy++, sx,   "                           {{{{{}}}}}}}.");
    mvwprintw(pWnd, sy++, sx,   "                          {{{{  {{{{{}}}}");
    mvwprintw(pWnd, sy++, sx,   "                         }}}}} _   _ {{{{{");
    mvwprintw(pWnd, sy++, sx,   "                         }}}}  6   6  }}}}");
    mvwprintw(pWnd, sy++, sx,   "                        {{{{C    ^    {{{{");
    mvwprintw(pWnd, sy++, sx,   "                       }}}}}}\\  '='  /}}}}}");
    mvwprintw(pWnd, sy++, sx,   "                       {{{{{{{;.___.;}}}}}}");
    mvwprintw(pWnd, sy++, sx,   "                        {{{{{{{)   (}}}}}}'");
    mvwprintw(pWnd, sy++, sx,   "                         `\"\"'\"':   :'\"'\"'`");
    mvwprintw(pWnd, sy++, sx,   "                                `@`");
    
    sy = 3;
    wattroff(pWnd, A_BOLD);
    wattron(pWnd, COLOR_PAIR(SYNTAX_PAIR_NORMAL));
    mvwprintw(pWnd, sy++, sx,   " ");
    mvwprintw(pWnd, sy++, sx,   " ");
    mvwprintw(pWnd, sy++, sx,   "   TinyTapeout");
    mvwprintw(pWnd, sy++, sx,   " ");
    mvwprintw(pWnd, sy++, sx,   "   TT06 / TT07");
    mvwprintw(pWnd, sy++, sx+31,   "_   _");
    mvwprintw(pWnd, sy++, sx+31,   "6   6");
    mvwprintw(pWnd, sy++, sx+33,   "^");
    mvwprintw(pWnd, sy, sx,   "    L I S A");
    mvwprintw(pWnd, sy++, sx+29,   "\\  '='  /");
    mvwprintw(pWnd, sy, sx,   "    Debugger");
    mvwprintw(pWnd, sy++, sx+31,   ".___.");
    mvwprintw(pWnd, sy++, sx,   " ");
    mvwprintw(pWnd, sy, sx,   "                  jgs");
    mvwprintw(pWnd, sy++, sx+30,   " :   :");
    mvwprintw(pWnd, sy++, sx,   "                                `@`");
}

// vim: sw=4 ts=4 et
