// Copyright 2012 Rui Ueyama. Released under the MIT license.
//
// Modified 2014 by Ken Pettit for LISA architecture

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include "lisacc.h"

bool dumpstack = false;
bool dumpsource = true;

char *localFuncs[1024];
int nLocalFuncs = 0;

typedef struct lvar_s
{
    int     kind;
    int     size;
    int     stackPos;
    int     assigned;
    int     used;
    int     useBeforeAssignWarned;
    int     notUsedWarn;
    char *  file;
    int     line;
    char *  name;
} lvar_t;

typedef struct asm_line_s
{
    struct asm_line_s *pNext;
    struct asm_line_s *pPrev;
    char              *pLine;
    int                stackRelative;
} asm_line_t;

typedef struct label_ref_s
{
    struct label_ref_s  *pNext;
    asm_line_t          *pAsmLine;
    char                label[256];
    int                 refCount;
} label_ref_t;

// Define optimizations struct
typedef struct opt_s
{
    int         ads0;
    int         iftt;
    int         literal_init;
    int         logand_logor;
    int         struct_masking;
    int         label_jumps;
} opts_t;

typedef struct stack_frame_s
{
    Node       *func;
    char       *fname;
    asm_line_t *pAdsLine;
    asm_line_t *pAsmLines;
    asm_line_t *pDataLines;
    int         raDestroyed;
    int         ixDestroyed;
    int         retCount;
    int         localArea;
    int         stackPos;
    int         stackOps;
    int         nlvars;
    lvar_t      lvars[32];
    int         nparam;
    lvar_t      param[32];
    char        accVar[256];
    int         accVal;
    int         accOnStack;
    char        ixVar[256];
    int         accModified;
    int         ixModified;
    int         nlocalLabels;
    char       *localLabels[1024];
    int         nexternLabels;
    char       *externLabels[1024];
    int         preserveVars;
    int         lastSwapOptional;
    int         isTernary;
    int         emitCompZero;
    int         noBitShiftStruc;
    asm_line_t *pLastRetLine;
    asm_line_t *pLastSwapLine;
    label_ref_t *pLabelRefs;
    opts_t      opts;
} stack_frame_t;

static int TAB = 8;
static Vector *functions = &EMPTY_VECTOR;
static int numgp;
static int numfp;
static FILE *outputfp;
static Map *source_files = &EMPTY_MAP;
static Map *source_lines = &EMPTY_MAP;
static char *last_loc = "";
static char *gMaybeEmitLoc = "";
static char *gMaybeEmitLine = "";
static const char *gpCurrSegment = "";
static int gLastEmitWasRet = 0;
static int gLastEmitWasJal = 0;
static stack_frame_t *pFrame = NULL;
static int gEmitToDataSection = 0;
extern char gOptimizationLevel;

static void emit_addr(Node *node);
static int emit_expr(Node *node);
static void emit_decl_init(Vector *inits, int off, int totalsize);
static void do_emit_data(Vector *inits, int size, int off, int depth);
static void emit_data(Node *v, int off, int depth);
void do_node2s(Buffer *b, Node *node, int indent);

#define REGAREA_SIZE 176

#define emit(...)        emitf(__LINE__, "    " __VA_ARGS__)
#define emit_noindent(...)  emitf(__LINE__, __VA_ARGS__)

#ifdef __GNUC__
#define SAVE                                                            \
    int save_hook __attribute__((unused, cleanup(pop_function)));       \
    if (dumpstack)                                                      \
        vec_push(functions, (void *)__func__);

static void pop_function(void *ignore) {
    if (dumpstack)
        vec_pop(functions);
}
#else
#define SAVE
#endif

static char *get_caller_list() {
    Buffer *b = make_buffer();
    for (int i = 0; i < vec_len(functions); i++) {
        if (i > 0)
            buf_printf(b, " -> ");
        buf_printf(b, "%s", vec_get(functions, i));
    }
    buf_write(b, '\0');
    return buf_body(b);
}

void set_output_file(FILE *fp) {
    outputfp = fp;
}

void close_output_file() {
    fclose(outputfp);
}

/*
==========================================================================================
Add an asm_line_t to the end of the linked list with the text pStr
==========================================================================================
*/
void add_asm_line(char *pStr)
{
    asm_line_t *pLine = (asm_line_t *) malloc(sizeof(asm_line_t));

    // Setup the asm line
    pLine->pNext = NULL;
    pLine->pPrev = NULL;
    pLine->pLine = strdup(pStr);
    pLine->stackRelative = 0;

    // Add it to the stack frame
    if (pFrame->pAsmLines == NULL)
    {
        // Add it as the first entry
        pFrame->pAsmLines = pLine;
        pLine->pNext = pLine;
        pLine->pPrev = pLine;
    }
    else
    {
        // Add it to the tail
        pLine->pNext = pFrame->pAsmLines;
        pLine->pPrev = pFrame->pAsmLines->pPrev;
        pFrame->pAsmLines->pPrev = pLine;
        pLine->pPrev->pNext = pLine;
    }
}

/*
==========================================================================================
Add a new line to the .data section linked list.  We don't want these interleaved
with the .code section, so we store them and emit them at the end of the current function.
==========================================================================================
*/
void add_data_line(char *pStr)
{
    asm_line_t *pLine = (asm_line_t *) malloc(sizeof(asm_line_t));

    // Setup the asm line
    pLine->pNext = NULL;
    pLine->pPrev = NULL;
    pLine->pLine = strdup(pStr);

    // Add it to the stack frame
    if (pFrame->pDataLines == NULL)
    {
        // Add it as the first entry
        pFrame->pDataLines = pLine;
        pLine->pNext = pLine;
        pLine->pPrev = pLine;
    }
    else
    {
        // Add it to the tail
        pLine->pNext = pFrame->pDataLines;
        pLine->pPrev = pFrame->pDataLines->pPrev;
        pFrame->pDataLines->pPrev = pLine;
        pLine->pPrev->pNext = pLine;
    }
}

/*
==========================================================================================
Delete the specified ASM line
==========================================================================================
*/
void delete_asm_line(asm_line_t *pLine)
{
    // Free the line text
    free(pLine->pLine);

    // Remove it from the linked list
    pLine->pPrev->pNext = pLine->pNext;
    pLine->pNext->pPrev = pLine->pPrev;

    if (pLine == pFrame->pAsmLines)
        pFrame->pAsmLines = pLine->pNext;
    free(pLine);
}

/*
==========================================================================================
Insert an asm_line_t before the specified pBefore asm_line
==========================================================================================
*/
void insert_asm_line_before(asm_line_t *pLine, asm_line_t *pBefore)
{
    // Insert pLine into the linked list
    pLine->pNext = pBefore;
    pLine->pPrev = pBefore->pPrev;
    pBefore->pPrev->pNext = pLine;
    pBefore->pPrev = pLine;

    // Test if pBefore was the first item
    if (pBefore == pFrame->pAsmLines)
        pFrame->pAsmLines = pLine;
}

/*
==========================================================================================
Move the given asm_line to after the specified pAfter line
==========================================================================================
*/
void move_asm_line_after(asm_line_t *pLine, asm_line_t *pAfter)
{
    // First remove pLine from the linked list
    if (pLine == pFrame->pAsmLines)
        pFrame->pAsmLines = pLine->pNext;
    pLine->pPrev->pNext = pLine->pNext;
    pLine->pNext->pPrev = pLine->pPrev;

    // Now insert the pLine after the pAfter line
    pLine->pNext = pAfter->pNext;
    pAfter->pNext = pLine;
    pLine->pPrev = pAfter;
    pLine->pNext->pPrev = pLine;
}

/*
==========================================================================================
Get the next asm line after the give line
==========================================================================================
*/
asm_line_t * get_next_asm_line(asm_line_t *pLine)
{
    asm_line_t  *pNext;

    if (pLine == NULL)
        return NULL;

    pNext = pLine->pNext;

    while (pNext != pFrame->pAsmLines)
    {
        // Test for comment or .loc lines
        if (!(strncmp(pNext->pLine, "    .", 5) == 0 ||
            strncmp(pNext->pLine, "    //", 6) == 0))
        {
            return pNext;
        }

        pNext = pNext->pNext;
    }

    return NULL;
}

/*
==========================================================================================
Get the previous asm line before the give line
==========================================================================================
*/
asm_line_t * get_prev_asm_line(asm_line_t *pLine)
{
    asm_line_t  *pPrev;

    if (pLine == NULL)
        return NULL;

    pPrev = pLine->pPrev;

    while (pPrev != pFrame->pAsmLines)
    {
        // Test for comment or .loc lines
        if (!(strncmp(pPrev->pLine, "    .", 5) == 0 ||
            strncmp(pPrev->pLine, "    //", 6) == 0))
        {
            return pPrev;
        }

        // Test for label
        if (strncmp(pPrev->pLine, "_L", 2) == 0)
            return NULL;

        pPrev = pPrev->pPrev;
    }

    return NULL;
}

/*
==========================================================================================
Get the next asm line after the give line
==========================================================================================
*/
asm_line_t * get_next_asm_line_no_label(asm_line_t *pLine)
{
    asm_line_t  *pNext;

    if (pLine == NULL)
        return NULL;

    pNext = pLine->pNext;

    while (pNext != pFrame->pAsmLines)
    {
        // Test for comment or .loc lines
        if (!(strncmp(pNext->pLine, "    .", 5) == 0 ||
            strncmp(pNext->pLine, "    //", 6) == 0 ||
            pNext->pLine[0] != ' '))
        {
            return pNext;
        }

        pNext = pNext->pNext;
    }

    return NULL;
}

/*
==========================================================================================
Get the last asm line with an opcode, skipping comments and .loc / .file directives
==========================================================================================
*/
asm_line_t * get_last_asm_line(void)
{
    asm_line_t  *pLine;

    pLine = pFrame->pAsmLines->pPrev;

    while (pLine != pFrame->pAsmLines)
    {
        // Test for comment or .loc lines
        if (!(strncmp(pLine->pLine, "    .", 5) == 0 ||
            strncmp(pLine->pLine, "    //", 6) == 0))
        {
            return pLine;
        }

        pLine = pLine->pPrev;
    }

    return NULL;
}

/*
==========================================================================================
Calculate the jal distance to it's label
==========================================================================================
*/
int calc_jal_distance(asm_line_t *pLine)
{
    asm_line_t  *pNext;
    asm_line_t  *pPrev;
    char        label[256];
    int         distance;

    if (pLine == NULL)
        return 999999;

    // Generate the label search text
    sprintf(label, "%s:", &pLine->pLine[14]);

    // Search forward for the label
    pNext = pLine->pNext;
    distance = 0;
    while (pNext != pFrame->pAsmLines)
    {
        // Test if this is the label
        if (strcmp(pNext->pLine, label) == 0)
            return distance;

        // Test for non-asm line
        if (pNext->pLine[0] != ' ' ||
            (!(strncmp(pNext->pLine, "    .", 5) == 0 ||
               strncmp(pNext->pLine, "    //", 6) == 0)))
        {
            distance++;
        }

        // Get next line
        pNext = pNext->pNext;
    }

    // Search forward for the label
    pPrev = pLine->pPrev;
    distance = 0;
    while (pPrev != pFrame->pAsmLines)
    {
        // Test if this is the label
        if (strcmp(pPrev->pLine, label) == 0)
            return distance;

        // Test for non-asm line
        if (pPrev->pLine[0] != ' ' ||
            (!(strncmp(pPrev->pLine, "    .", 5) == 0 ||
               strncmp(pPrev->pLine, "    //", 6) == 0)))
        {
            distance--;
        }

        // Get next line
        pPrev = pPrev->pPrev;
    }

    return 99999;
}

/*
==========================================================================================
Set the name of the current variable in IX
==========================================================================================
*/
static void set_ix_var(char *varname, ...)
{
    char    sLine[512];
    va_list args;

    va_start(args, varname);
    vsprintf(sLine, varname, args);
    va_end(args);

    // Test if acc has a modified var

    // Set the acc var
    strcpy(pFrame->ixVar, sLine);
    pFrame->ixModified = 0;
}

/*
==========================================================================================
Clear the variable in IX
==========================================================================================
*/
static void clear_ix_var(void)
{
    // Test if IX has a modified var

    // Clear the acc var
    pFrame->ixVar[0] = 0;
    pFrame->ixModified = 0;
}

/*
==========================================================================================
Set the name of the current variable in ACC / 1(sp)
==========================================================================================
*/
static void set_acc_var(char *varname)
{
    // Test if acc has a modified var

    // Set the acc var
    strcpy(pFrame->accVar, varname);
    pFrame->accModified = 0;
}

/*
==========================================================================================
Mark the ACC variable as damaged / no loaded variable name
==========================================================================================
*/
static void clear_acc_var(void)
{
    pFrame->accVal = -1000;
    if (pFrame->accVar[0] == 0)
        return;

    // Test if acc has a modified var and save it if needed
    
    // Clear the accVar
    pFrame->accVar[0] = 0;
    pFrame->accModified = 0;
}

/*
==========================================================================================
Test if the given string is a load ACC opcode
==========================================================================================
*/
static bool isAccLoad(char *op) {
    if (strncmp(op, "lda", 3) == 0 ||
        strncmp(op, "ldi", 3) == 0 ||
        strncmp(op, "txa", 3) == 0 ||
        strncmp(op, "reti", 4) == 0 ||
        strncmp(op, "pop_a", 5) == 0)
    {
        return true;
    }
    return false;
}

/*
==========================================================================================
Test if the given string is an opcode that uses the value in ACC
==========================================================================================
*/
static bool isAccUse(char *op) {
    if (strncmp(op, "sta", 3) == 0 ||
        strncmp(op, "ret", 3) == 0 ||
        strncmp(op, "rc", 2)  == 0 ||
        strncmp(op, "rz", 2)  == 0 ||
        strncmp(op, "adc", 3) == 0 ||
        strncmp(op, "shl", 3) == 0 ||
        strncmp(op, "shr", 3) == 0 ||
        strncmp(op, "tax", 3) == 0 ||
        strncmp(op, "cpi", 3) == 0 ||
        strncmp(op, "add", 3) == 0 ||
        strncmp(op, "mul", 3) == 0 ||
        strncmp(op, "sub", 3) == 0 ||
        strncmp(op, "or",  2) == 0 ||
        strncmp(op, "xor", 3) == 0 ||
        strncmp(op, "cmp", 3) == 0)
    {
        return true;
    }

    return false;
}

/*
==========================================================================================
Do a formatted emit of opcode, label, etc.
==========================================================================================
*/
static void emitf(int line, char *fmt, ...) {
    // Replace "#" with "%%" so that vfprintf prints out "#" as "%".
    char buf[256];
    char sLine[512];
    char retTest[256];
    int i = 0;

    for (char *p = fmt; *p; p++) {
        assert(i < sizeof(buf) - 3);
        if (*p == '#' && *(p+1) == '#') {
            p++;
            buf[i++] = *p;
        }
        else if (*p == '#') {
            buf[i++] = '%';
            buf[i++] = '%';
        } else {
            buf[i++] = *p;
        }
    }
    buf[i] = '\0';

#if 0
    if (strlen(gMaybeEmitLoc) > 0)
      fprintf(outputfp, "\n%s\n", gMaybeEmitLoc);
    gMaybeEmitLoc = "";
    if (strlen(gMaybeEmitLine) > 0)
      fprintf(outputfp, "%s\n", gMaybeEmitLine);
    gMaybeEmitLine = "";
#endif

    /* Test if we can remove the last optional swap */
    if (pFrame->lastSwapOptional)
    {
        /* Test for opcodes that load acc */
        if (isAccLoad(&fmt[4]))
        {
            /* Delete the last optional swap */
            delete_asm_line(pFrame->pLastSwapLine);
            pFrame->pLastSwapLine = NULL;
            pFrame->lastSwapOptional = 0;
        }

        /* Test for opcodes that use acc after the last optional swap */
        else if (isAccUse(&fmt[4]))
        {
            /* Last optional swap actually needed. */
            pFrame->pLastSwapLine = NULL;
            pFrame->lastSwapOptional = 0;
        }
    }

    va_list args;
    va_start(args, fmt);
    int col = vsprintf(sLine, buf, args);
    va_end(args);

    if (dumpstack) {
        for (char *p = fmt; *p; p++)
            if (*p == '\t')
                col += TAB - 1;
        int space = (28 - col) > 0 ? (30 - col) : 2;
        sprintf(&sLine[strlen(sLine)], "%*c %s:%d", space, '#', get_caller_list(), line);
    }
    if (gEmitToDataSection)
        add_data_line(sLine);
    else
        add_asm_line(sLine);

    sprintf(retTest, "    jal       _L%s_ret", pFrame->fname);
    if (strncmp(sLine, "    .", 5) != 0)
    {
        gLastEmitWasRet = strcmp(sLine, retTest) == 0;
        if (gLastEmitWasRet)
            pFrame->pLastRetLine = pFrame->pAsmLines->pPrev;
        else
            pFrame->pLastRetLine = NULL;
        gLastEmitWasJal = strncmp(sLine, "    jal", 7) == 0;
        if (gLastEmitWasJal)
            pFrame->raDestroyed = 1;
    }
}

static void emit_nostack(char *fmt, ...) {
    char  sLine[512];
    sprintf(sLine, "    ");
    va_list args;
    va_start(args, fmt);
    vsprintf(&sLine[strlen(sLine)], fmt, args);
    va_end(args);
    add_asm_line(sLine);
}

/*
==========================================================================================
Test if the given name is an LVAR and return it's offset or -1 if not LVAR.
==========================================================================================
*/
static int find_lvar_offset(char *pVarName)
{
  int   x;

  // Test if it is a local
  for (x = 0; x < pFrame->nlvars; x++)
  {
    if (strcmp(pFrame->lvars[x].name, pVarName) == 0)
      return x;
  }

  return -1;
}

/*
==========================================================================================
Test if the given name is a function param and return it's offset or -1 if not LVAR.
==========================================================================================
*/
static int find_param_offset(char *pVarName)
{
  int   x;

  // Test if it is a local
  for (x = 0; x < pFrame->nparam; x++)
  {
    if (strcmp(pFrame->param[x].name, pVarName) == 0)
      return x;
  }

  return -1;
}

/*
==========================================================================================
Mark the curent function as using a minimum of depth bytes of stack.  The function
stack depth can grow but not shrink.

If growth occurs, walk through all existing asm_lines to find any with #(sp) arguments
and add the growth to the index offset numbers (for any line marked as stackRelative).
==========================================================================================
*/
static void mark_stack_operations(int depth)
{
  char        str[32];
  asm_line_t  *pLine;
  int           diff;

  if (pFrame->stackOps >= depth)
    return;

  // Mark stack as using stack ops
  diff = depth - pFrame->stackOps;
  pFrame->stackOps = depth;
  
  // Add 2 to stackPos to account for 16-bit op
  pFrame->localArea += diff;

  // Change the ads line to add 2 
  free(pFrame->pAdsLine->pLine);
  sprintf(str, "    ads       %d", -pFrame->localArea);
  pFrame->pAdsLine->pLine = strdup(str);

  // Add two to all stack relative operations
  pLine = pFrame->pAdsLine->pNext;
  while (pLine != pFrame->pAsmLines)
  {
    char *pSt;

    // Test if this is a stack relative access
    if (pLine->stackRelative)
    {
      int   off;
      int   idx = 14;
      pSt = strstr(pLine->pLine, "(sp)");

      if (pSt)
          *pSt = 0;
      strncpy(str, pLine->pLine, idx);
      str[idx] = 0;
      if (pLine->pLine[idx] == '$')
      {
        idx++;
        strcat(str, "$");
      }

      off = atoi(&pLine->pLine[idx]);
      if (pSt)
          sprintf(&str[idx], "%d(sp)", off + diff);
      else
          sprintf(&str[idx], "%d", off + diff);
      free(pLine->pLine);
      pLine->pLine = strdup(str);
    }

    // Point to next line
    pLine = pLine->pNext;
  }

  // Loop for all frame params and add 2 to their offset
  for (int i = 0; i < pFrame->nparam; i++) {
      Node *v = vec_get(pFrame->func->params, i);
      v->loff += diff;
      pFrame->param[i].stackPos += diff;
  }

  // Loop for all frame lvars and add 2 to their offset
  for (int i = 0; i < pFrame->nlvars; i++) {
      Node *v = vec_get(pFrame->func->localvars, i);
      v->loff += diff;
      pFrame->lvars[i].stackPos += diff;
  }
}

static int align(int n, int m) {
    int rem = n % m;
    return (rem == 0) ? n : n - rem + m;
}

static void push_xmm(int reg) {
    SAVE;
    emit("sub $8, #rsp");
    emit("movsd #xmm%d, (#rsp)", reg);
//    stackPos += 8;
}

static void pop_xmm(int reg) {
    SAVE;
    emit("movsd (#rsp), #xmm%d", reg);
    emit("add $8, #rsp");
//    stackPos -= 8;
//    assert(stackPos >= 0);
}

/*
==========================================================================================
Push the specified register to the stack, keeping track of stackPos location to the
generator can correctly calculate LVAR and param stack offsets.
==========================================================================================
*/
static void push(char *reg) {
    SAVE;
    if (strcmp(reg, "ix") == 0)
    {
      emit("push_ix");
      pFrame->stackPos += 2;
    }
    else if (strcmp(reg, "a") == 0)
    {
      emit("push_a");
      pFrame->stackPos += 1;
    }
    else
    {
      emit("push      %s", reg);
      pFrame->stackPos += 8;
    }
}

/*
==========================================================================================
Pop the specified register from the stack, keeping track of stackPos.
==========================================================================================
*/
static void pop(char *reg) {
    SAVE;
    if (strcmp(reg, "ix") == 0)
    {
      emit("pop_ix");
      pFrame->stackPos -= 2;
    }
    else if (strcmp(reg, "a") == 0)
    {
      emit("pop       a");
      pFrame->stackPos -= 1;
      pFrame->accVal = -10000;
    }
    else
    {
      emit("pop       %s", reg);
      pFrame->stackPos -= 8;
    }
}

/*
==========================================================================================
Push a struct to the stack.  This is generally a bad idea in LISA architecture unless
it is a very small struct.
==========================================================================================
*/
static int push_struct(int size) {
    SAVE;
    int aligned = align(size, 8);
    emit("sub $%d, #rsp", aligned);
    emit("mov #rcx, -8(#rsp)");
    emit("mov #r11, -16(#rsp)");
    emit("mov #rax, #rcx");
    int i = 0;
    for (; i < size; i += 8) {
        emit("movq %d(#rcx), #r11", i);
        emit("mov #r11, %d(#rsp)", i);
    }
    for (; i < size; i += 4) {
        emit("movl %d(#rcx), #r11", i);
        emit("movl #r11d, %d(#rsp)", i);
    }
    for (; i < size; i++) {
        emit("movb %d(#rcx), #r11", i);
        emit("movb #r11b, %d(#rsp)", i);
    }
    emit("mov -8(#rsp), #rcx");
    emit("mov -16(#rsp), #r11");
//    stackPos += aligned;
    return aligned;
}

static void maybe_emit_bitshift_load(Type *ty) {
    SAVE;
    if (ty->bitsize <= 0)
        return;
    int x;
    int mask;
    for (x = 0; x < ty->bitoff; x++)
    {
        if (ty->size == 1)
            emit("shr");
        else
            emit("shr16");
        pFrame->accVal = -10000;
    }

    mask = (1 << ty->bitsize) - 1;
    emit("andi      %d", mask & 0xFF);
    if (ty->size > 1)
    {
        emit("swap      1(sp)");
        emit("andi      %d", (mask >> 8) & 0xFF);
        emit("swap      1(sp)");
        pFrame->accVal = -10000;
    }
}

static int maybe_emit_bitshift_save(Type *ty, int off, char *idx, char *modifier) {
    SAVE;
    int     x;

    if (ty->bitsize <= 0)
        return 0;

    if (!pFrame->noBitShiftStruc)
    {
        emit("andi      %d", ((1 << (long)ty->bitsize) - 1) & 0xFF);
        pFrame->accVal = -10000;
        for (x = 0; x < ty->bitoff; x++)
        {
            if (ty->size == 1)
                emit("shl");
            else
                emit("shl16");
        }
    }
    emit("swap      %s%d(%s)", modifier, off, idx);
    pFrame->pAsmLines->pPrev->stackRelative = 1;
    emit("andi      %d", ~(((1 << (long)ty->bitsize) - 1) << ty->bitoff) & 0xFF);
    emit("or        %s%d(%s)", modifier, off, idx);
    pFrame->pAsmLines->pPrev->stackRelative = 1;
    emit("stax      %s%d(%s)", modifier, off, idx);
    pFrame->pAsmLines->pPrev->stackRelative = 1;

    if (ty->size > 1 && ty->bitsize + ty->bitoff > 8)
    {
        emit("ldax      1(%s)", idx);
        emit("andi      %d", (((1 << (long)ty->bitsize) - 1) >> 8) & 0xFF);
        emit("swap      %s%d(%s)", modifier, off, idx);
        pFrame->pAsmLines->pPrev->stackRelative = 1;
        emit("andi      %d", (~(((1 << (long)ty->bitsize) - 1) << ty->bitoff) >> 8) & 0xFF);
        emit("or        %s%d(%s)", modifier, off, idx);
        pFrame->pAsmLines->pPrev->stackRelative = 1;
        emit("stax      %s%d(%s)", modifier, off, idx);
        pFrame->pAsmLines->pPrev->stackRelative = 1;
    }
    clear_acc_var();
    return 1;
}

/*
==========================================================================================
Emit code to save a global variable (access, SFR, other).
==========================================================================================
*/
static void emit_gload(Type *ty, char *label, int off) {
    SAVE;
    char        str[512];

    // Test if acc already has this var
    if (strcmp(pFrame->accVar, label) == 0)
        return;
    clear_acc_var();

    if (ty->kind == KIND_ARRAY) {
        sprintf(str, "&%s", label);
        if (strcmp(str, pFrame->ixVar) != 0)
        {
            emit("spix");
            if (off)
            {
                emit("adx       %d", off);
                pFrame->pAsmLines->pPrev->stackRelative = 1;
            }
            set_ix_var(str);
        }
        return;
    }

    if (ty->issfr)
    {
        set_acc_var(label);
        emit("lda       %s", label);
    }
    else if (ty->isaccess)
    {
        set_acc_var(label);
        emit("lda       %s", label);
    }
    else
    {
        set_acc_var(label);
        sprintf(str, "&%s", label);
        if (strcmp(str, pFrame->ixVar) != 0)
        {
            emit("ldx       %s", label);
            set_ix_var("&%s", label);
        }
        if (ty->size == 2)
        {
            emit("ldax      1(ix)");
            emit("stax      1(sp)");
            mark_stack_operations(2);
        }
        emit("ldax      0(ix)");
    }
    maybe_emit_bitshift_load(ty);
}

/*
==========================================================================================
Add an extern label line at the top of the function
==========================================================================================
*/
static void emit_extern(char *pStr) {
    int         x;
    asm_line_t  *pLine;
    char        str[256];

    for (x = 0; x < pFrame->nexternLabels; x++)
    {
        if (strcmp(pFrame->externLabels[x], pStr) == 0)
            return;
    }

    // Add the extern label
    pFrame->externLabels[pFrame->nexternLabels++] = strdup(pStr);

    sprintf(str, "    .extern %s", pStr);
    pLine = (asm_line_t *) malloc(sizeof(asm_line_t));
    pLine->pLine = strdup(str);
    insert_asm_line_before(pLine, pFrame->pAsmLines->pNext);
}

/*
==========================================================================================
Emit code to convert to an int.
==========================================================================================
*/
static void emit_intcast(Type *ty) {
    char    func[16];
    char    fromSign = ty->usig ? 'u' : 's';

    switch(ty->kind) {
    case KIND_BOOL:
    case KIND_CHAR:
        sprintf(func, "__%cctoint", fromSign);
        emit("jal       %s", func);
        emit_extern(func);
        return;
    case KIND_INT:
        return;
    case KIND_LONG:
        sprintf(func, "__%cltoint", fromSign);
        emit("jal       %s", func);
        emit_extern(func);
        return;
    case KIND_LLONG:
        return;
    }
}

/*
==========================================================================================
Convert a floating point type to an int
==========================================================================================
*/
static void emit_toint(Type *ty) {
    SAVE;
    if (ty->kind == KIND_FLOAT)
        emit("cvttss2si #xmm0, #eax");
    else if (ty->kind == KIND_DOUBLE)
        emit("cvttsd2si #xmm0, #eax");
}

/*
==========================================================================================
Emit code to load a local variable (LVAR or func param).
==========================================================================================
*/
static void emit_lload(Node* node, Type *ty, char *base, int off) {
    SAVE;
    int     stackOff;
    int     isSP;
    char    varName[256];
    
    if (strcmp(base, "sp") == 0)
    {
        isSP = 1;
        stackOff = pFrame->stackPos;
    }
    else
    {
        isSP = 0;
        stackOff = 0;
    }

    // Test if acc already has this var
    if (ty->kind != KIND_PTR && strcmp(pFrame->accVar, node->varname) == 0)
        return;

    if (ty->kind == KIND_PTR && strcmp(pFrame->ixVar, node->varname) == 0)
        return;

    if (pFrame->emitCompZero)
    {
        if (ty->kind == KIND_PTR)
            clear_ix_var();
        else
            clear_acc_var();
    }
    else
    {
        if (ty->kind == KIND_PTR)
            set_ix_var(node->varname);
        else
            set_acc_var(node->varname);
    }

    if (ty->kind == KIND_ARRAY) {
        sprintf(varName, "&%s", node->varname);
        if (strcmp(varName, pFrame->ixVar) != 0)
        {
            if (strcmp(base, "sp") == 0)
                emit("spix");
            emit("adx       %d", off + pFrame->stackPos);
            pFrame->pAsmLines->pPrev->stackRelative = 1;
            set_ix_var(varName);
        }
    } else if (ty->kind == KIND_PTR) {
        emit("ldxx      %s%d(sp)", ty->isparam ? "$" : "", off);
        if (isSP)
            pFrame->pAsmLines->pPrev->stackRelative = 1;
        strcpy(pFrame->ixVar, node->varname);
    } else if (ty->kind == KIND_DOUBLE || ty->kind == KIND_LDOUBLE) {
        emit("movsd %d(#%s), #xmm0", off, base);
    } else {
        int  lvarIdx = find_lvar_offset(node->varname);
        char modifier[2] = {0,};

        if (lvarIdx == -1)
        {
          // See if it is a parameter
          lvarIdx = find_param_offset(node->varname);
          if (lvarIdx != -1)
            modifier[0] = '$';
        }

        if (ty->size == 2)
        {
          emit("ldax      %s%d(%s)", modifier, off+stackOff+1, base);
          pFrame->accVal = -10000;
          if (isSP)
            pFrame->pAsmLines->pPrev->stackRelative = 1;
          if (!pFrame->emitCompZero)
          {
              emit("stax      1(sp)");
              mark_stack_operations(2);
              if (isSP)
                  off += 2;
          }
        }

        if (pFrame->emitCompZero)
            emit("or        %s%d(%s)", modifier, off+stackOff, base);
        else
            emit("ldax      %s%d(%s)", modifier, off+stackOff, base);
        if (isSP)
            pFrame->pAsmLines->pPrev->stackRelative = 1;
        pFrame->accVal = -10000;
        maybe_emit_bitshift_load(ty);
    }
}

/*
==========================================================================================
Convert a comparision result to a BOOL
==========================================================================================
*/
static void maybe_convert_bool(Type *ty) {
    if (ty->kind == KIND_BOOL) {
        emit("andi      0xFF");
        emit("if        nz");
        emit("ldi       1");
        pFrame->accVal = 1;
    }
}

/*
==========================================================================================
Emit code to save a global variable (SFR, access, other).
==========================================================================================
*/
static void emit_gsave(char *varname, Type *ty, int off) {
    SAVE;
    char    varName[256];

    assert(ty->kind != KIND_ARRAY);
    maybe_convert_bool(ty);

    set_acc_var(varname);
    if (ty->issfr | ty->isaccess)
    {
        emit("sta       %s\n", varname);
    }
    else
    {
        sprintf(varName, "&%s", varname);
        if (strcmp(varName, pFrame->ixVar) != 0)
        {
            emit("ldx       %s", varname);
            set_ix_var(varName);
        }
        emit("stax      0(ix)");
    }
}

/*
==========================================================================================
Emit code to save a local variable (LVAR or func param).
==========================================================================================
*/
static void emit_lsave(char *varname, Type *ty, int off) {
    SAVE;
    int idx = find_lvar_offset(varname);
    if (idx != -1)
        pFrame->lvars[idx].assigned = 1;
    if (ty->kind == KIND_DOUBLE) {
        emit("movsd #xmm0, %d(#rbp)", off);
    } else if (ty->kind == KIND_PTR) {
        emit("stxx      %s%d(sp)", ty->isparam ? "$" : "", off);
        pFrame->pAsmLines->pPrev->stackRelative = 1;
    } else {
        int  lvarIdx = find_lvar_offset(varname);
        int  offset = 0xFFFFFFFF;
        char modifier[2] = {0,};

        if (lvarIdx != -1)
          offset = pFrame->lvars[lvarIdx].stackPos + pFrame->stackPos;
        else
        {
          // See if it is a parameter
          lvarIdx = find_param_offset(varname);
          if (lvarIdx != -1)
          {
            offset = pFrame->param[lvarIdx].stackPos + pFrame->stackPos;
            modifier[0] = '$';
          }
        }

        if (ty->kind != KIND_FLOAT)
        {
            maybe_convert_bool(ty);
            if (maybe_emit_bitshift_save(ty, off, "sp", modifier))
                return;
        }

        if (offset != 0xFFFFFFFF)
        {
          emit("stax      %s%d(sp)", modifier, offset);
          pFrame->pAsmLines->pPrev->stackRelative = 1;
          if (pFrame->lvars[lvarIdx].size == 2)
          {
            emit("swap      1(sp)");
            emit("stax      %s%d(sp)", modifier, offset + 1);
            pFrame->pAsmLines->pPrev->stackRelative = 1;
            emit("swap      1(sp)");
            pFrame->lastSwapOptional = 1;
            pFrame->pLastSwapLine = pFrame->pAsmLines->pPrev;
            mark_stack_operations(2);
          }
        }
    }
    set_acc_var(varname);
}

/*
==========================================================================================
Generate code to DEREF store a variable (simple or struct)
==========================================================================================
*/
static void do_emit_assign_deref(Type *ty, int off) {
    if (pFrame->accOnStack)
    {
        emit("ldax      0(sp)");
        pFrame->accOnStack = 0;
    }
    emit("stax      %d(ix)", off);
    if (ty->ptr->size > 1)
    {
        emit("swap      1(sp)");
        emit("stax      %d(ix)", off + 1);
        emit("swap      1(sp)");
        pFrame->lastSwapOptional = 0;
    }
}

/*
==========================================================================================
Generate code to DEREF store a non-struct variable
==========================================================================================
*/
static void emit_assign_deref(Node *var, Node *simpleLoad) {
    SAVE;
    emit_expr(var->operand);
    if (simpleLoad)
        emit_expr(simpleLoad);
    do_emit_assign_deref(var->operand->ty, var->operand->ty->offset);
}

/*
==========================================================================================
Generate code for pointer arithemetic
==========================================================================================
*/
static void emit_pointer_arith(char kind, Node *left, Node *right) {
    SAVE;
    int     needPush = 1;
    int     rkind;
    int     size;
    const char   *op;

    rkind = right->kind;
    size = right->ty ? right->ty->size : 0;
    if ((rkind == AST_LITERAL || rkind == AST_LVAR ||
        (rkind == AST_GVAR && (right->ty->issfr || right->ty->isaccess))) &&
        size < 2)
    {
        needPush = 0;
    }
    emit_expr(left);
    if (needPush)
    {
        emit("stxx      0(sp)");
        emit("ads       -2");
        pFrame->stackPos += 2;
    }
    emit_expr(right);
    size = left->ty->ptr->size;
    if (size == 2 || size == 4)
    {
        if (right->ty->size == 1)
            emit("shl");
        else if (right->ty->size == 2)
            emit("shl16");
        pFrame->accVal = -10000;
    }
    else if (size > 1)
    {
        emit("stax      0(sp)");
        emit("ldi       %d", size);
        emit("mul       0(sp)");
        pFrame->accVal = -10000;
    }
    if (needPush)
    {
        emit("ads       2");
        emit("ldxx      0(sp)");
        pFrame->ixVar[0] = 0;
        pFrame->stackPos -= 2;
    }
    switch (kind) {
        case '+': op = "add"; break;
        case '-': op = "sub"; break;
        default: error("invalid pointer arith operator '%d'", kind);
    }

    if (size == 1 || size == 2 || size == 4)
    {
        if (size == 1 && right->ty->size == 1)
            emit("%sax     nc", op);
        else if (right->ty->size == 1)
        {
            emit("%sax     c", op);
            if (size == 4)
                emit("%sax     c", op);
        }
        else
        {
            emit("%sax     nc", op);
            emit("ldax      1(sp)");
            emit("%saxu", op);
        }
    }
    else
    {
        emit("%sax     nc", op);
        emit("ldi       %d", size);
        emit("mulu      0(sp)");
        emit("%saxu    nc", op);
    }
}

/*
==========================================================================================
Zero fill initialized variable data
==========================================================================================
*/
static void emit_zero_filler(int start, int end) {
    SAVE;
    int     size = end - start+1;

    if (start == end)
        return;
    if (size > 10)
    {
        emit("spix");
        if (start + size <= 123)
        {
            emit("adx       %d", start + size + pFrame->stackPos);
            pFrame->pAsmLines->pPrev->stackRelative = 1;
            size = 0;
        }
        else
        {
            emit("adx       %d", 123);
            size -= 123 - start;
        }
        while (size > 0)
        {
            emit("adx       %d", size > 123 ? 123 : size + pFrame->stackPos);
            if (size <= 123)
                pFrame->pAsmLines->pPrev->stackRelative = 1;
            size -= size > 123 ? 123 : size;
        }
        emit("xchg      ra");
        pFrame->raDestroyed = 1;
        emit("spix");
        emit("adx       %d", start + pFrame->stackPos);
        pFrame->pAsmLines->pPrev->stackRelative = 1;
        emit("ldi       0");
        emit("stax      0(ix)");
        emit("adx       1");
        emit("cpx       ra");
        emit("bnz       -3");
        pFrame->accVal = -1000;
    }
    else
    {
        emit("ldi       0");
        pFrame->accVal = 0;
        for (; start <= end; start++)
        {
            emit("stax      %d(sp)", start);
            pFrame->pAsmLines->pPrev->stackRelative = 1;
        }
    }
}

/*
==========================================================================================
Test if local variable needs to be initialized with inital data
==========================================================================================
*/
static void ensure_lvar_init(Node *node) {
    SAVE;
    assert(node->kind == AST_LVAR);
    if (node->lvarinit)
        emit_decl_init(node->lvarinit, node->loff, node->ty->size);
    node->lvarinit = NULL;
}

/*
==========================================================================================
Generate code to assign to a struct variable
==========================================================================================
*/
static void emit_assign_struct_ref(Node *struc, Type *field, int off) {
    SAVE;
    switch (struc->kind) {
    case AST_LVAR:
        ensure_lvar_init(struc);
        emit_lsave(struc->varname, field, struc->loff + field->offset + off);
        break;
    case AST_GVAR:
        emit_gsave(struc->glabel, field, field->offset + off);
        break;
    case AST_STRUCT_REF:
        emit_assign_struct_ref(struc->struc, field, off + struc->ty->offset);
        break;
    case AST_DEREF:
        emit_expr(struc->operand);
        do_emit_assign_deref(field, field->offset + off);
        break;
    default:
        error("internal error: %s", node2s(struc, 0));
    }
}

static void emit_load_struct_ref(Node* node, Node *struc, Type *field, int off) {
    SAVE;
    if (struc->ty->issfr)
        field->issfr = true;
    switch (struc->kind) {
    case AST_LVAR:
        ensure_lvar_init(struc);
        emit_lload(node, field, "sp", struc->loff + field->offset + off);
        break;
    case AST_GVAR:
        emit_gload(field, struc->glabel, field->offset + off);
        break;
    case AST_STRUCT_REF:
        emit_load_struct_ref(struc, struc->struc, field, struc->ty->offset + off);
        break;
    case AST_DEREF:
        emit_expr(struc->operand);
        emit_lload(node, field, "ix", field->offset + off);
        break;
    default:
        error("internal error: %s", node2s(struc, 0));
    }
}

/*
==========================================================================================
Generate code to save to a variable
==========================================================================================
*/
static void emit_store(Node *var, Node *simpleLoad) {
    SAVE;
    switch (var->kind) {
    case AST_DEREF: emit_assign_deref(var, simpleLoad); break;
    case AST_STRUCT_REF: emit_assign_struct_ref(var->struc, var->ty, 0); break;
    case AST_LVAR:
        ensure_lvar_init(var);
        emit_lsave(var->varname, var->ty, var->loff);
        break;
    case AST_GVAR: emit_gsave(var->glabel, var->ty, 0); break;
    default: error("internal error");
    }
}

static void emit_to_bool(Type *ty) {
    SAVE;
    if (is_flotype(ty)) {
        push_xmm(1);
        emit("xorpd #xmm1, #xmm1");
        emit("%s #xmm1, #xmm0", (ty->kind == KIND_FLOAT) ? "ucomiss" : "ucomisd");
        emit("setne #al");
        pop_xmm(1);
    } else {
        emit("cmp $0, #rax");
        emit("setne #al");
    }
    emit("movzb #al, #eax");
}

/*
==========================================================================================
Generate code to perform a BOOL compare of a value:   "if (a)" syntax
==========================================================================================
*/
static void maybe_emit_bool_compare(Node *node) {
    SAVE;
    asm_line_t *pLine;

    // Test if left expression is just a variable name
    if (node->kind == AST_LVAR || node->kind == AST_GVAR)
    {
        if (node->ty && node->ty->size == 1)
            emit("if        z");
        else
        {
            // Test for ldax/stax load of int
            pLine = get_last_asm_line();
            if (strncmp(&pLine->pLine[4], "ldax", 4) == 0 &&
                strcmp(pLine->pPrev->pLine, "    stax      1(sp)") == 0)
            {
                // Convert the ldax line to "or" and delete the stax 1(sp)
                pLine->pLine[4] = 'o';
                pLine->pLine[5] = 'r';
                pLine->pLine[6] = ' ';
                pLine->pLine[7] = ' ';
                delete_asm_line(pLine->pPrev);
            }
            else
            {
                // Two byte compare
                emit("or        1(sp)");
            }
            pFrame->accVal = -1000;
            emit("if        z");
            clear_acc_var();
        }
    }
}

/*
==========================================================================================
Add a code to perform a JUMP/GOTO
==========================================================================================
*/
static void emit_jmp(char *label) {
    emit("jal       %s", label);
    pFrame->raDestroyed = 1;
}

/*
==========================================================================================
Generate code for comparisions
==========================================================================================
*/
static int emit_comp(char *usiginst, char *inst, Node *node) {
    char    str[4];
    char    lbl[32];

    switch (node->kind)
    {
        case '<': strcpy(str, node->left->ty->usig ? "ge" : "sge"); break;
        case '>': strcpy(str, node->left->ty->usig ? "le" : "sle"); break;
        case OP_EQ: strcpy(str, "ne"); break;
        case OP_LE: strcpy(str, node->left->ty->usig ? "gt" : "sgt"); break;
        case OP_NE: strcpy(str, "eq"); break;
        case OP_GE: strcpy(str, node->left->ty->usig ? "lt" : "slt"); break;
    }

    SAVE;
    if (is_flotype(node->left->ty)) {
        emit_expr(node->left);
        push_xmm(0);
        emit_expr(node->right);
        pop_xmm(1);
        if (node->left->ty->kind == KIND_FLOAT)
            emit("ucomiss #xmm0, #xmm1");
        else
            emit("ucomisd #xmm0, #xmm1");
    } else {
        if (node->right->kind == AST_LITERAL)
        {
            emit_expr(node->left);
            if (node->left->ty->kind == KIND_CHAR && node->right->ty->kind == KIND_CHAR)
            {
                emit("cpi       %d", node->right->ival);
                emit("if        %s", str);
                return 0;
            }
            else
            {
                // Handle lt, le, gt, ge
                emit("stax      0(sp)");
                emit("ads       -2");
                pFrame->stackPos += 2;
                emit_expr(node->right);

                sprintf(lbl, "__cmpint%s", str);
                emit_extern("__cmpint");
                pFrame->accVal = -1000;
                emit_jmp(lbl);
                emit("if        nz");
                pFrame->stackPos -= 2;
                //emit("if        nc");
                emit_extern(lbl);
                return 0;
            }
        }
        else if (node->left->ty->kind == KIND_PTR)
        {
            emit_expr(node->left);
            emit("xchg      ra");
            pFrame->raDestroyed = 1;
            emit_expr(node->right);
            emit("cpx       ra");
            emit("if        %s", str);
            return 0;
        }
        else if (node->left->ty->size == 1 &&
                 node->right->ty->size == 1)
        {
            asm_line_t *pLine;

            emit_expr(node->left);
            emit_expr(node->right);
            pLine = get_last_asm_line();
            pLine->pLine[4] = 'c';
            pLine->pLine[5] = 'm';
            pLine->pLine[6] = 'p';
            pLine->pLine[7] = ' ';
            emit("if        %s", str);
        }
        else
        {
            emit_expr(node->left);
            emit("stax      0(sp)");
            emit("ads       -2");
            pFrame->stackPos += 2;

            emit_expr(node->right);

            sprintf(lbl, "__cmpint%s", str);
            emit_extern("__cmpint");
            emit_jmp(lbl);
            pFrame->accVal = -1000;
            emit("if        nz");
            pFrame->stackPos -= 2;
            emit_extern(lbl);
            return 0;
        }
    }

    return 0;
}

/*
==========================================================================================
Perform integer shift operations
==========================================================================================
*/
static void emit_binop_int_shift(Node *node) {
    char  *op = NULL;
    int     x;
    int     dist;

    switch (node->kind) {
        case OP_SAL: op = "shl"; break;
        case OP_SHR: op = "shr"; break;
        case OP_SAR: op = "shr"; break;
    }

    // Test if right-hand side is LITERAL
    if (node->right->kind == AST_LITERAL)
    {
        emit_expr(node->left);

        if (node->kind == OP_SAR)
            emit("amode     2");
        if (node->left->ty->kind == KIND_CHAR)
        {
            dist = node->right->ival;
            if (dist > 8)
                dist = 8;
            else if (dist > 0)
            {
                for (x = 0; x < node->right->ival; x++)
                    emit("%s", op);
            }
        }
        else
        {
            // Must be an int
            if (node->right->ival <= 6)
            {
                for (x = 0; x < node->right->ival; x++)
                {
                    if (node->kind == OP_SAL)
                        emit("shl16     1");    // Perform the shift ...
                    else
                        emit("shr16     1");
                }
                pFrame->accVal = -1000;
            }
            else
            {
                emit("stax      0(sp)");    // Store LSB
                emit("ldi       %d", node->right->ival & 0xf);
                // Swap with MSB
                emit("swap      0(sp)");    // Restore LSB and save shift count
                pFrame->lastSwapOptional = 0;
                pFrame->pLastSwapLine = NULL;
                emit("dcx       0(sp)");    // Decrement shift count
                emit("iftt      nc");       // If no underflow ...
                if (node->kind == OP_SAL)
                    emit("shl16     1");    // Perform the shift ...
                else
                    emit("shr16     1");
                emit("br        -3");       // ... and branch to maybe shift again
                mark_stack_operations(2);
                pFrame->accVal = -1000;
            }
        }
        if (node->kind == OP_SAR)
            emit("amode     0");
    }

    // Test if left-hand side is LITERAL
    else if (node->left->kind == AST_LITERAL)
    {
        // Evaluate right-hand side first
        emit_expr(node->right);
        emit("stax      0(sp)");

        // Get literal value
        emit_expr(node->left);

        // Test for arithemetic shift
        if (node->kind == OP_SAR)
            emit("amode     2");

        // Perform variable shift
        if (node->left->ty->kind == KIND_CHAR)
        {
            emit("dcx       0(sp)");
            emit("iftt      nc");
            emit("%s", op);
            emit("br        -3");
        }
        else
        {
            emit("dcx       0(sp)");
            emit("iftt      nc");
            if (node->kind == OP_SAL)
                emit("shl16     1");
            else
                emit("shr16     1");
            emit("br        -3");
        }
        pFrame->accVal = -1000;

        if (node->kind == OP_SAR)
            emit("amode     0");
    }
    else
    {
        printf("ERROR! %s:%d  Need to add support for non literal shift of non literal!!!\n",
                __FILE__, __LINE__);
    }
}

/*
==========================================================================================
Generate code for ADD operation
==========================================================================================
*/
static void emit_add(Node *node) {
    int     kind;

    SAVE;

    // Test for simple add of literal value
    if (node->left->kind == AST_LVAR && node->left->ty->kind == KIND_CHAR &&
         node->right->kind == AST_LITERAL && node->right->ty->kind == KIND_CHAR &&
         strcmp(node->left->varname, pFrame->accVar) != 0)
    {
        int  lvarIdx = find_lvar_offset(node->left->varname);
        int  offset = 0xFFFFFFFF;
        char modifier[2] = {0,};

        if (lvarIdx != -1)
          offset = pFrame->lvars[lvarIdx].stackPos + pFrame->stackPos;
        else
        {
          // See if it is a parameter
          lvarIdx = find_param_offset(node->left->varname);
          if (lvarIdx != -1)
          {
            offset = pFrame->param[lvarIdx].stackPos + pFrame->stackPos;
            modifier[0] = '$';
          }
        }

        emit("ldi       %d", node->right->ival);
        emit("add       %s%d(sp)", modifier, offset+pFrame->stackPos);
        pFrame->accVal = -1000;
        pFrame->pAsmLines->pPrev->stackRelative = 1;
//        emit("stax      %s%d(sp)", modifier, offset+pFrame->stackPos);
//        pFrame->pAsmLines->pPrev->stackRelative = 1;
        return;
    }

    emit_expr(node->left);

    // Test for simple literal add
    if (node->right->kind == AST_LITERAL)
    {
        if (node->ty->kind == KIND_INT || node->ty->kind == KIND_SHORT)
        {
            emit("stax      0(sp)");
            emit("ldi       %d", (node->right->ival >> 8) & 0xFF);
            emit("swap      0(sp)");        // 0(sp) now has MSB of literal
        }
        else
            emit("ldc       0");

        // Perform simple immediate add
        emit("adc       %d", node->right->ival & 0xFF);

        // Test for 16 bit add
        if (node->ty->kind == KIND_INT || node->ty->kind == KIND_SHORT)
        {
            emit("swap      1(sp)");        // Save LSB, get MSB
            emit("add       0(sp)");        // Add MSB of Literal
            emit("swap      1(sp)");        // Save MSB, get LSB
            pFrame->lastSwapOptional = 0;
            pFrame->pLastSwapLine = NULL;
            mark_stack_operations(2);
        }
        pFrame->accVal = -1000;
        return;
    }
    
    // Test for right hand AST_LVAR or AST_GVAR
    kind = node->right->kind;
    if (kind == AST_LVAR || kind == AST_GVAR)
    {
        if (kind == AST_LVAR)
        {
            emit("ldc       0");
            if (node->right->ty->isparam)
                emit("add       $%d(sp)", node->right->loff + pFrame->stackPos);
            else
                emit("add       %d(sp)", node->right->loff + pFrame->stackPos);
            pFrame->pAsmLines->pPrev->stackRelative = 1;
            if (node->ty->kind == KIND_SHORT || node->ty->kind == KIND_INT)
            {
                emit("swap      1(sp)");        // Save LSB, get MSB
                if (node->right->ty->isparam)
                    emit("add       $%d(sp)", node->right->loff + pFrame->stackPos+1);
                else
                    emit("add       %d(sp)", node->right->loff + pFrame->stackPos+1);
                pFrame->pAsmLines->pPrev->stackRelative = 1;
                emit("swap      1(sp)");        // Save MSB, get LSB
                pFrame->lastSwapOptional = 0;
                pFrame->pLastSwapLine = NULL;
                mark_stack_operations(2);
            }
        }
        else
        {
            if (node->right->ty->issfr)
            {
                emit("stax      0(sp)");
                emit("lda       %s", node->right->varname);
                emit("ldc       0");        // Ensure C is clear
                emit("add       0(sp)");
                mark_stack_operations(2);

                // Test for 16-bit add
                if (node->ty->kind == KIND_SHORT || node->ty->kind == KIND_INT)
                {
                    emit("stax      0(sp)");    // Save LSB
                    emit("andi      0");        // Zero acc, keeping C flag
                    emit("add       1(sp)");    // Add zero + c to MSB
                    emit("stax      1(sp)");    // Save MSB
                    emit("ldax      0(sp)");    // Get LSB
                }
            }
            else
            {
                emit("stax      0(sp)");
                emit("lda       %s", node->right->varname);
                emit("ldc       0");        // Ensure C is clear
                emit("add       0(sp)");
                mark_stack_operations(2);

                // Test for 16-bit add
                if (node->right->ty->kind == KIND_SHORT || node->right->ty->kind == KIND_INT)
                {
                    emit("swap      1(sp)");    // Save LSB
                    emit("ldx       %s", node->right->varname);
                    emit("add       1(ix)");    // Add zero + c to MSB
                    emit("swap      1(sp)");    // Save MSB
                    pFrame->lastSwapOptional = 0;
                    pFrame->pLastSwapLine = NULL;
                    set_ix_var(node->right->varname);
                }
            }
        }
        pFrame->accVal = -1000;
    }
    else
    {
        // Save LSB to stack and advance by 2
        emit("stax      0(sp)");
        emit("ads       -2");
        pFrame->stackPos += 2;
        emit_expr(node->right);
        emit_jmp("__addint");
        emit_extern("__addint");
        pFrame->stackPos -= 2;
        pFrame->accVal = -1000;
    }
}

/*
==========================================================================================
Generate code for SUB operation
==========================================================================================
*/
static void emit_sub(Node *node) {
    int     kind;

    SAVE;

    emit_expr(node->left);

    // Test for simple literal add
    if (node->right->kind == AST_LITERAL)
    {
        if (node->ty->kind == KIND_INT || node->ty->kind == KIND_SHORT)
        {
            emit("stax      0(sp)");
            emit("ldi       %d", ((-node->right->ival) >> 8) & 0xFF);
            emit("swap      0(sp)");        // 0(sp) now has MSB of literal
        }
        else
            emit("ldc       0");

        // Perform simple immediate add of the negative value
        emit("adc       %d", (-node->right->ival) & 0xFF);

        // Test for 16 bit add
        if (node->ty->kind == KIND_INT || node->ty->kind == KIND_SHORT)
        {
            emit("swap      1(sp)");        // Save LSB, get MSB
            emit("add       0(sp)");        // Add MSB of Literal
            emit("swap      1(sp)");        // Save MSB, get LSB
            pFrame->lastSwapOptional = 0;
            pFrame->pLastSwapLine = NULL;
            mark_stack_operations(2);
        }
        pFrame->accVal = -1000;
        return;
    }
    
    // Test for right hand AST_LVAR or AST_GVAR
    kind = node->right->kind;
    if (kind == AST_LVAR || kind == AST_GVAR)
    {
        if (kind == AST_LVAR)
        {
            asm_line_t *pLine = get_last_asm_line();
            if (strncmp(&pLine->pLine[4], "ldi", 3) != 0)
                emit("ldc       0");
            if (node->right->ty->isparam)
                emit("sub       $%d(sp)", node->right->loff + pFrame->stackPos);
            else
                emit("sub       %d(sp)", node->right->loff + pFrame->stackPos);
            pFrame->pAsmLines->pPrev->stackRelative = 1;
            if (node->ty->kind == KIND_SHORT || node->ty->kind == KIND_INT)
            {
                emit("swap      1(sp)");        // Save LSB, get MSB
                if (node->right->ty->isparam)
                    emit("sub       $%d(sp)", node->right->loff + pFrame->stackPos+1);
                else
                    emit("sub       %d(sp)", node->right->loff + pFrame->stackPos+1);
                pFrame->pAsmLines->pPrev->stackRelative = 1;
                emit("swap      1(sp)");        // Save MSB, get LSB
                pFrame->lastSwapOptional = 0;
                pFrame->pLastSwapLine = NULL;
                mark_stack_operations(2);
            }
        }
        else
        {
            if (node->right->ty->issfr)
            {
                emit("stax      0(sp)");
                emit("lda       %s", node->right->varname);
                emit("ldc       0");        // Ensure C is clear
                emit("sub       0(sp)");
                mark_stack_operations(2);

                // Test for 16-bit add
                if (node->ty->kind == KIND_SHORT || node->ty->kind == KIND_INT)
                {
                    emit("stax      0(sp)");    // Save LSB
                    emit("andi      0");        // Zero acc, keeping C flag
                    emit("sub       1(sp)");    // Add zero + c to MSB
                    emit("stax      1(sp)");    // Save MSB
                    emit("ldax      0(sp)");    // Get LSB
                }
            }
            else
            {
                emit("stax      0(sp)");
                emit("lda       %s", node->right->varname);
                emit("ldc       0");        // Ensure C is clear
                emit("sub       0(sp)");
                mark_stack_operations(2);

                // Test for 16-bit add
                if (node->right->ty->kind == KIND_SHORT || node->right->ty->kind == KIND_INT)
                {
                    emit("swap      1(sp)");    // Save LSB
                    emit("ldx       %s", node->right->varname);
                    emit("sub       1(ix)");    // Add zero + c to MSB
                    emit("swap      1(sp)");    // Save MSB
                    pFrame->lastSwapOptional = 0;
                    pFrame->pLastSwapLine = NULL;
                    set_ix_var(node->right->varname);
                }
            }
        }
        pFrame->accVal = -1000;
    }
    else
    {
        // Save LSB to stack and advance by 2
        emit("stax      0(sp)");
        emit("ads       -2");
        pFrame->stackPos += 2;
        emit_expr(node->right);
        emit_jmp("__subint");
        emit_extern("__subint");
        pFrame->stackPos -= 2;
        pFrame->accVal = -1000;
    }
}

/*
==========================================================================================
Perform integer multiplication
==========================================================================================
*/
static void emit_binop_int_mult(Node *node) {
    if (node->ty->size == 1 &&
        node->left->ty->size == 1 &&
        node->right->ty->size == 1)
    {
        emit_expr(node->left);
        emit("stax      0(sp)");
        if (!(node->right->kind == AST_LVAR ||
              node->right->kind == AST_LVAR ||
              node->right->kind == AST_LITERAL))
        {
            emit("ads       -1");
            pFrame->stackPos += 1;
            mark_stack_operations(1);
        }
        emit_expr(node->right);
        if (!(node->right->kind == AST_LVAR ||
              node->right->kind == AST_LVAR ||
              node->right->kind == AST_LITERAL))
        {
            emit("ads       1");
            pFrame->stackPos -= 1;
        }
        emit("mul       0(sp)");
        pFrame->accVal = -1000;
    }
    else
    {
        emit_expr(node->left);
        emit("stax      0(sp)");

        if (node->right->ty->size == 1)
        {
            emit_expr(node->right);
            if (node->left->ty->usig && node->right->ty->usig)
            {
                emit_jmp("__umulintchar");
                emit_extern("__umulintchar");
            }
            else
            {
                emit_jmp("__smulintchar");
                emit_extern("__smulintchar");
            }
        }
        else
        {
            emit("ads       -2");
            pFrame->stackPos += 2;
            mark_stack_operations(2);
            emit_expr(node->right);
            if (node->left->ty->usig && node->right->ty->usig)
            {
                emit_jmp("__umulint");
                emit_extern("__umulint");
            }
            else
            {
                emit_jmp("__smulint");
                emit_extern("__smulint");
            }
            emit("ads       2");
            pFrame->stackPos -= 2;
        }
        pFrame->accVal = -1000;
    }
}

/*
==========================================================================================
Perform integer arithemetic / shift operations
==========================================================================================
*/
static void emit_binop_int_arith(Node *node) {
    SAVE;
    clear_acc_var();
    switch (node->kind) {
    case '+': return emit_add(node);
    case '-': return emit_sub(node);
    case '*': return emit_binop_int_mult(node); break;
    case OP_SAL: 
    case OP_SAR:
    case OP_SHR: 
        return emit_binop_int_shift(node);

    case '/': case '%': break;
    default:
      {
        Buffer *b = make_buffer();
        do_node2s(b, node, 0);
        printf("WARNING:  invalid binop_int_arith operator '%s'\n", buf_body(b));
        return;
      }
    }

    if (node->right->ty->size == 1 &&
        node->left->ty->size == 1 &&
        node->left->ty->usig == node->right->ty->usig)
    {
        // Put divisor on the stack
        emit_expr(node->right);
        if (!(node->left->kind == AST_LVAR ||
              node->left->kind == AST_GVAR ||
              node->left->kind == AST_LITERAL))
        {
            push("a");
        }
        else
            emit("stax      0(sp)");

        // Load dividend
        emit_expr(node->left);

        if (!(node->left->kind == AST_LVAR ||
              node->left->kind == AST_GVAR ||
              node->left->kind == AST_LITERAL))
        {
            emit("ads       1");
            pFrame->stackPos--;
        }

        if (node->kind == '/')
        {
            // Perform the character divide
            if (node->left->ty->usig && node->right->ty->usig)
            {
                emit_jmp("__udivchch");
                emit_extern("__udivchch");
            }
            else
            {
                emit_jmp("__sdivchch");
                emit_extern("__sdivchch");
            }
        }
        else
        {
            // Perform the character remainder
            if (node->left->ty->usig && node->right->ty->usig)
            {
                emit_jmp("__uremchch");
                emit_extern("__uremchch");
            }
            else
            {
                emit_jmp("__sremchch");
                emit_extern("__sremchch");
            }
        }
        pFrame->accVal = -1000;
    }
    else
    {
        // Put divisor on the stack
        emit_expr(node->right);
        if (node->right->ty->size == 1)
        {
            if (node->right->ty->usig)
            {
                emit_jmp("__uctoint");
                emit_extern("__uctoint");
            }
            else
            {
                emit_jmp("__sctoint");
                emit_extern("__sctoint");
            }
        }
        emit("stax      0(sp)");
        emit("ads       -2");
        mark_stack_operations(2);

        // Load dividend
        emit_expr(node->left);
        if (node->left->ty->size == 1)
        {
            if (node->left->ty->usig)
            {
                emit_jmp("__uctoint");
                emit_extern("__uctoint");
            }
            else
            {
                emit_jmp("__sctoint");
                emit_extern("__sctoint");
            }
        }

        if (node->kind == '/')
        {
            // Perform divide
            if (node->left->ty->usig && node->right->ty->usig)
            {
                emit_jmp("__udivint");
                emit_extern("__udivint");
            }
            else
            {
                emit_jmp("__sdivint");
                emit_extern("__sdivint");
            }
        }
        else
        {
            // Perform remainer operation
            if (node->left->ty->usig && node->right->ty->usig)
            {
                emit_jmp("__uremint");
                emit_extern("__uremint");
            }
            else
            {
                emit_jmp("__sremint");
                emit_extern("__sremint");
            }
        }
    }

    clear_ix_var();
    clear_acc_var();
}

static void emit_binop_float_arith(Node *node) {
    SAVE;
    char *op;
    bool isdouble = (node->ty->kind == KIND_DOUBLE);
    switch (node->kind) {
    case '+': op = (isdouble ? "addsd" : "addss"); break;
    case '-': op = (isdouble ? "subsd" : "subss"); break;
    case '*': op = (isdouble ? "mulsd" : "mulss"); break;
    case '/': op = (isdouble ? "divsd" : "divss"); break;
    default: error("invalid binop_float_arith operator '%d'", node->kind);
    }
    emit_expr(node->left);
    push_xmm(0);
    emit_expr(node->right);
    emit("%s #xmm0, #xmm1", (isdouble ? "movsd" : "movss"));
    pop_xmm(0);
    emit("%s #xmm1, #xmm0", op);
}

static void emit_load_convert(Type *to, Type *from) {
    SAVE;
    if (is_inttype(from) && to->kind == KIND_FLOAT)
        emit("cvtsi2ss #eax, #xmm0");
    else if (is_inttype(from) && to->kind == KIND_DOUBLE)
        emit("cvtsi2sd #eax, #xmm0");
    else if (from->kind == KIND_FLOAT && to->kind == KIND_DOUBLE)
        emit("cvtps2pd #xmm0, #xmm0");
    else if ((from->kind == KIND_DOUBLE || from->kind == KIND_LDOUBLE) && to->kind == KIND_FLOAT)
        emit("cvtpd2ps #xmm0, #xmm0");
    else if (to->kind == KIND_BOOL)
        emit_to_bool(from);
    else if (is_chartype(from) && is_chartype(to))
        ;
    else if (is_inttype(from) && is_inttype(to))
        emit_intcast(from);
    else if (is_inttype(to))
        emit_toint(from);
}

/*
==========================================================================================
Generate code to emit a RETurn
==========================================================================================
*/
static void emit_ret(int localArea, Type *ty, int raDestroyed) {
    asm_line_t  *pLine;
    int         lastWasLocalJal = 0;
    int         jalToLastLabel = 0;
    int         x;
    char        lastLabel[256] = {0,};

    SAVE;
    if (gLastEmitWasRet)
    {
        //pFrame->retCount--;
        if (pFrame->retCount > 1)
            sprintf(pFrame->pAsmLines->pPrev->pLine, "_L%s_ret:", pFrame->fname);
        else
        {
            // Simply delete the last line
            delete_asm_line(pFrame->pLastRetLine);
        }
    }
    else if (pFrame->retCount)
        emit_noindent("_L%s_ret:", pFrame->fname);

    // Test for a local jump to the last label
    pLine = pFrame->pAsmLines->pPrev;
    while (pLine != pFrame->pAsmLines)
    {
        if (strlen(lastLabel) == 0)
        {
            // Test for jal after last label
            if (strncmp(pLine->pLine, "    jal", 7) == 0)
                break;

            // Find first line with a label
            if (strchr(pLine->pLine, ':') != NULL)
            {
                strcpy(lastLabel, pLine->pLine);
                for (x = 0; x < strlen(lastLabel); x++)
                    if (lastLabel[x] == ':')
                    {
                        lastLabel[x] = 0;
                        break;
                    }
            }
        }
        else
        {
            // Test for jal to the lastLabel
            if (strncmp(pLine->pLine, "    jal", 7) == 0)
            {
                if (strcmp(&pLine->pLine[14], lastLabel) == 0)
                {
                    jalToLastLabel = 1;
                    break;
                }
            }
        }

        // Test if we rewound beyond the function entry
        if (strstr(pLine->pLine, ".public") != NULL ||
            strstr(pLine->pLine, ".extern") != NULL)
        {
            break;
        }

        // Next previous line
        pLine = pLine->pPrev;
    }

    // Test if last opcode was a ret to a local label
    if (!jalToLastLabel)
    {
        pLine = pFrame->pAsmLines->pPrev;
        while (pLine != pFrame->pAsmLines)
        {
            // Skip any label lines
            if (strchr(pLine->pLine, ':') == NULL)
            {
                // Test for jal line
                if (strncmp(pLine->pLine, "    jal", 7) == 0)
                {
                    // Test if jump label is a local label
                    for (x = 0; x < pFrame->nlocalLabels; x++)
                        if (strcmp(&pLine->pLine[14], pFrame->localLabels[x]) == 0)
                        {
                            lastWasLocalJal = 1;
                            break;
                        }
                }
        
                break;
            }
        
            // Next previous line
            pLine = pLine->pPrev;
        }
    }

    if (pFrame->retCount > 0 || !lastWasLocalJal)
    {
        if (localArea)
        {
            // Test if returning a mult-byte return value
            if (ty->size > 1)
            {
                // Returning multi-byte return value
                if (pFrame->retCount < 2)
                {
                    asm_line_t *pLine = get_last_asm_line();
                    if (pLine != NULL)
                    {
                        char    str[32];

                        // Test for stax
                        if (strcmp(&pLine->pLine[4], "stax      1(sp)") == 0)
                        {
                            // Last opcode was store of MSB.  Simply change
                            // the offset to the return value offset
                            sprintf(str, "    stax      %d(sp)", 1 + localArea + raDestroyed * 2);
                            free(pLine->pLine);
                            pLine->pLine = strdup(str);
                        }
                        // Test for swap
                        else if (strcmp(&pLine->pLine[4], "swap      1(sp)") == 0)
                        {
                            // Insert store of MSB prior to swap with LSB
                            asm_line_t *pStaxLine = (asm_line_t *) malloc(sizeof(asm_line_t));
                            sprintf(str, "    stax      %d(sp)", 1 + localArea + raDestroyed * 2);

                            // Setup the asm line
                            pStaxLine->pNext = NULL;
                            pStaxLine->pPrev = NULL;
                            pStaxLine->pLine = strdup(str);
                            insert_asm_line_before(pStaxLine, pLine);
                        }
                    }
                }
                else
                {
                    emit("swap      1(sp)");
                    emit("stax      %d(sp)", 1+localArea + raDestroyed * 2);
                    emit("swap      1(sp)");
                }
            }
            emit("ads       %d", localArea);
        }

        if (ty->isisr)
            emit("rets");
        else
            emit("ret");
        pFrame->retCount++;
    }

    // Test for unused or assigned but not used variables
    for (x = 0; x < pFrame->nlvars; x++)
    {
        char    *file = pFrame->lvars[x].file;
        int     line = pFrame->lvars[x].line;
        // Test for unused variables
        if (pFrame->lvars[x].used == 0 && pFrame->lvars[x].assigned == 0)
        {
            printf("%s:%d: Warning: Unused variable %s\n", file, line, pFrame->lvars[x].name);
        }
        else if (pFrame->lvars[x].used == 0)
        {
            printf("%s:%d: Warning: Variable set but never used: %s\n", file, line, pFrame->lvars[x].name);
        }
    }
}

/*
==========================================================================================
Generate code for binary operations (comparisons)
==========================================================================================
*/
static int emit_binop(Node *node) {
    SAVE;
    if (node->ty->kind == KIND_PTR) {
        emit_pointer_arith(node->kind, node->left, node->right);
        return 0;
    }
    switch (node->kind) {
    case '<': return emit_comp("lt", "slt", node);
    case '>': return emit_comp("gt", "sgt", node);
    case OP_EQ: return emit_comp("eq", "eq", node);
    case OP_LE: return emit_comp("le", "sle", node);
    case OP_NE: return emit_comp("ne", "ne", node);
    case OP_GE: return emit_comp("ge", "sge", node);
    }
    if (is_inttype(node->ty))
        emit_binop_int_arith(node);
    else if (is_flotype(node->ty))
        emit_binop_float_arith(node);
    else
        error("internal error: %s", node2s(node, 0));
    return 0;
}

/*
==========================================================================================
Generate code to save a LITERAL to a variable.
==========================================================================================
*/
static void emit_save_literal(Node *node, Type *totype, int off) {
    switch (totype->kind) {
    case KIND_BOOL:
        emit("ldi       %d", !!node->ival);
        emit("stax      %d(sp)", off + pFrame->stackPos);
        pFrame->pAsmLines->pPrev->stackRelative = 1;
        break;
    case KIND_CHAR:
        if (pFrame->accVal != node->ival)
            emit("ldi       %d", node->ival);
        emit("stax      %d(sp)", off + pFrame->stackPos);
        pFrame->accVal = node->ival;
        pFrame->pAsmLines->pPrev->stackRelative = 1;
        break;
    case KIND_SHORT:
    case KIND_INT:
        emit("ldi       %d", (node->ival >> 8) & 0xFF);
        emit("stax      %d(sp)", off+1 + pFrame->stackPos);
        pFrame->pAsmLines->pPrev->stackRelative = 1;
        emit("ldi       %d", node->ival & 0xFF);
        emit("stax      %d(sp)", off + pFrame->stackPos);
        pFrame->accVal = node->ival & 0xFF;
        pFrame->pAsmLines->pPrev->stackRelative = 1;
        break;
    case KIND_LONG:
    case KIND_LLONG:
    case KIND_PTR: {
        emit("movl $%lu, %d(#rbp)", ((uint64_t)node->ival) & ((1L << 32) - 1), off);
        emit("movl $%lu, %d(#rbp)", ((uint64_t)node->ival) >> 32, off + 4);
        break;
    }
    case KIND_FLOAT: {
        float fval = node->fval;
        emit("movl $%u, %d(#rbp)", *(uint32_t *)&fval, off);
        break;
    }
    case KIND_DOUBLE:
    case KIND_LDOUBLE: {
        emit("movl $%lu, %d(#rbp)", *(uint64_t *)&node->fval & ((1L << 32) - 1), off);
        emit("movl $%lu, %d(#rbp)", *(uint64_t *)&node->fval >> 32, off + 4);
        break;
    }
    default:
        error("internal error: <%s> <%s> <%d>", node2s(node, 0), ty2s(totype), off);
    }
}

/*
==========================================================================================
Generate code to take the address of a variable
==========================================================================================
*/
static void emit_addr(Node *node)
{
    char    varName[256];

    switch (node->kind) {
    case AST_LVAR:
        ensure_lvar_init(node);
        sprintf(varName, "&%s", node->varname);
        if (strcmp(varName, pFrame->ixVar) != 0)
        {
            emit("spix");
            emit("adx       %s%d", node->isParam ? "$" : "", node->loff + pFrame->stackPos);
            pFrame->pAsmLines->pPrev->stackRelative = 1;
            set_ix_var(varName);
            pFrame->pAsmLines->pPrev->stackRelative = 1;
        }
        break;
    case AST_GVAR:
        if (node->ty->issfr)
            error("Can't take address of an SFR!: %s", node2s(node, 0));
        emit("ldx       %s", node->glabel);
        set_ix_var(node->glabel);
        break;
    case AST_DEREF:
        emit_expr(node->operand);
        break;
    case AST_STRUCT_REF:
        emit_addr(node->struc);
        emit("add $%d, #rax", node->ty->offset);
        break;
    case AST_FUNCDESG:
        emit("ldx       %s", node->fname);
        set_ix_var(node->fname);
        break;
    default:
        error("internal error: %s", node2s(node, 0));
    }
}

static void emit_copy_struct(Node *left, Node *right) {
    push("rcx");
    push("r11");
    emit_addr(right);
    emit("mov #rax, #rcx");
    emit_addr(left);
    int i = 0;
    for (; i < left->ty->size; i += 8) {
        emit("movq %d(#rcx), #r11", i);
        emit("movq #r11, %d(#rax)", i);
    }
    for (; i < left->ty->size; i += 4) {
        emit("movl %d(#rcx), #r11", i);
        emit("movl #r11, %d(#rax)", i);
    }
    for (; i < left->ty->size; i++) {
        emit("movb %d(#rcx), #r11", i);
        emit("movb #r11, %d(#rax)", i);
    }
    pop("r11");
    pop("rcx");
}

static int cmpinit(const void *x, const void *y) {
    Node *a = *(Node **)x;
    Node *b = *(Node **)y;
    return a->initoff - b->initoff;
}

/*
==========================================================================================
Generate code for to zero fill holes of non specified initialized variables in an
initialized array.
==========================================================================================
*/
static void emit_fill_holes(Vector *inits, int off, int totalsize) {
    // If at least one of the fields in a variable are initialized,
    // unspecified fields has to be initialized with 0.
    int len = vec_len(inits);
    Node **buf = malloc(len * sizeof(Node *));
    for (int i = 0; i < len; i++)
        buf[i] = vec_get(inits, i);
    qsort(buf, len, sizeof(Node *), cmpinit);

    int lastend = 0;
    for (int i = 0; i < len; i++) {
        Node *node = buf[i];
        if (lastend < node->initoff)
        {
            if (node->initval->ival == 0)
            {
                emit_zero_filler(lastend + off, node->initoff + off +
                        node->initval->ty->size);
                node->initval->initialized = true;
            }
            else
                emit_zero_filler(lastend + off, node->initoff + off);
        }
        lastend = node->initoff + node->totype->size;
    }
    emit_zero_filler(lastend + off, totalsize + off);
}

/*
==========================================================================================
Generate code for initialized variable declartion
==========================================================================================
*/
static void emit_decl_init(Vector *inits, int off, int totalsize) {
    emit_fill_holes(inits, off, totalsize);
    for (int i = 0; i < vec_len(inits); i++) {
        Node *node = vec_get(inits, i);
        assert(node->kind == AST_INIT);
        bool isbitfield = (node->totype->bitsize > 0);
        if (node->initval->initialized)
            continue;
        if (node->initval->kind == AST_LITERAL && !isbitfield) {
            emit_save_literal(node->initval, node->totype, node->initoff + off);
        } else {
            emit_expr(node->initval);
            emit_lsave(node->varname, node->totype, node->initoff + off);
        }
    }
}

/*
==========================================================================================
Generate code for pre increment / decrement operations
==========================================================================================
*/
static void emit_pre_inc_dec(Node *node, char *op) {
    emit_expr(node->operand);
    emit("%s $%d, #rax", op, node->ty->ptr ? node->ty->ptr->size : 1);
    emit_store(node->operand, NULL);
}

/*
==========================================================================================
Generate code for post increment / decrement operations
==========================================================================================
*/
static void emit_post_inc_dec(Node *node, char *op) {
    SAVE;
    if (node->ty->ptr)
    {
        emit_expr(node->operand);
        if (strcmp(op, "add") == 0)
            emit("adx       1");
        else
            emit("adx       -1");
    }
    else
    {
        Node *n = node->operand;
        Node *s = NULL;
        int offset = 0;
        char    xop[4];
        
        if (strcmp("add", op) == 0)
            strcpy(xop, "inx");
        else
            strcpy(xop, "dcx");

        if (n->kind == AST_STRUCT_REF && n->ty->bitsize == -1)
        {
            s = n->struc;
            offset = s->ty->offset;

            while (s->kind == AST_STRUCT_REF && s->ty->bitsize == -1)
            {
                s = s->struc;
                offset += s->ty->offset;    
            }
        }

        if (!n->ty->isaccess &&
            ((n->kind == AST_LVAR || n->kind == AST_GVAR) ||
             (n->kind == AST_STRUCT_REF && n->ty->bitsize == -1 &&
              (s->kind == AST_LVAR || s->kind == AST_GVAR)))
             )
        {
            int   off = n->kind == AST_STRUCT_REF ? n->struc->loff : n->loff;

            // Use the inx opcode
            // Load 1 to ACC and clear C flag
            if (n->kind == AST_LVAR || (n->kind == AST_STRUCT_REF && s->kind == AST_LVAR))
            {
                // Local stack variable 
                emit("%s       %s%d(sp)", xop, n->isParam ? "$" : "",
                        off + pFrame->stackPos + n->ty->offset);
                pFrame->pAsmLines->pPrev->stackRelative = 1;
                if (n->ty->size > 1)
                {
                    emit("if        c");
                    emit("%s       %s%d(sp)", xop, n->isParam ? "$" : "",
                            off + pFrame->stackPos + n->ty->offset + 1);
                    pFrame->pAsmLines->pPrev->stackRelative = 1;
                }

                if (strcmp(n->varname, pFrame->accVar) == 0)
                    clear_acc_var();
            }
            else
            {
                // Global variable
                emit("ldx       %s", n->slabel);
                clear_ix_var();
                emit("%s       %d(ix)", xop, n->ty->offset);
                if (n->ty->size > 1)
                {
                    emit("if        c");
                    emit("%s       %d(ix)", xop, n->ty->offset + 1);
                }

                if (strcmp(n->varname, pFrame->accVar) == 0)
                    clear_acc_var();
            }
            return;
        }
        else if ((n->kind == AST_DEREF &&
                 (n->operand->kind == AST_LVAR ||
                  n->operand->kind == AST_GVAR)) || 
                 (n->kind == AST_STRUCT_REF && s &&
                  s->kind == AST_DEREF &&
                  n->ty->bitsize == -1 &&
                  (s->operand->kind == AST_LVAR ||
                   s->operand->kind == AST_GVAR)))
        {
            int   off = n->kind == AST_STRUCT_REF ? s->operand->loff : n->operand->loff;

            //emit_expr(node->operand);
            emit("ldxx      %s%d(sp)", n->isParam ? "$" : "", off + pFrame->stackPos);
            clear_ix_var();
            pFrame->pAsmLines->pPrev->stackRelative = 1;
            emit("%s       %d(ix)", xop, n->ty->offset + offset);
            if (n->ty->size > 1)
            {
                emit("if        c");
                emit("%s       %d(ix)", xop, n->ty->offset + offset+1);
            }
            return;
        }

        else
        {
            emit_expr(node->operand);
            emit("ldc       0");
            if (strcmp(op, "add") == 0)
                emit("adc       1");
            else
                emit("adc       -1");
            
            if (node->ty->kind == KIND_INT || node->ty->kind == KIND_SHORT)
            {
                emit("bnc       6");
                emit("stax      0(sp)");
                emit("ldi       1");
                emit("add       1(sp)");
                emit("stax      1(sp)");
                emit("ldax      0(sp)");
            }
            pFrame->accVal = node->ival;
        }
    }
    emit_store(node->operand, NULL);
}

static void emit_je(char *label) {
    emit("if        z");
}

/*
==========================================================================================
Add a local label line at the assembly
==========================================================================================
*/
static void emit_label(char *label) {
    label_ref_t *pRef;

    pFrame->localLabels[pFrame->nlocalLabels++] = label;
    emit_noindent("%s:", label);
    if (!pFrame->preserveVars)
    {
        set_acc_var("");
        set_ix_var("");
    }
    pFrame->preserveVars = 0;

    // Create a label ref
    pRef = (label_ref_t *) malloc(sizeof(label_ref_t));
    pRef->refCount = 0;
    pRef->pNext = pFrame->pLabelRefs;
    pRef->pAsmLine = pFrame->pAsmLines->pPrev;
    strcpy(pRef->label, label);
    pFrame->pLabelRefs = pRef;
}

/*
==========================================================================================
Generate code to load a literal value
==========================================================================================
*/
static void emit_literal(Node *node) {
    SAVE;
    pFrame->accVar[0] = 0;
    switch (node->ty->kind) {
    case KIND_BOOL:
    case KIND_CHAR:
        if (pFrame->accVal != node->ival)
            emit("ldi       %d", node->ival);
        pFrame->accVal = node->ival;
        break;
    case KIND_SHORT:
    case KIND_INT:
        if (pFrame->accVal != ((node->ival >> 8) & 0xFF))
            emit("ldi       %u", (node->ival >> 8) & 0xFF);
        emit("swap      1(sp)");
        pFrame->lastSwapOptional = 0;
        pFrame->pLastSwapLine = NULL;
        emit("ldi       %u", node->ival & 0xFF);
        pFrame->accVal = node->ival & 0xFF;
        mark_stack_operations(2);
        break;
    case KIND_LONG:
    case KIND_LLONG: {
        emit("mov $%lu, #rax", node->ival);
        break;
    }
    case KIND_FLOAT: {
        if (!node->flabel) {
            node->flabel = make_label();
            float fval = node->fval;
            if (strcmp(gpCurrSegment, ".data") != 0)
            {
              gpCurrSegment = ".data";
              emit_noindent("\n    .section .data");
            }
            emit_label(node->flabel);
            emit(".long %d", *(uint32_t *)&fval);
            emit_noindent("\n    .section .text");
        }
        emit("movss %s(#rip), #xmm0", node->flabel);
        break;
    }
    case KIND_DOUBLE:
    case KIND_LDOUBLE: {
        if (!node->flabel) {
            node->flabel = make_label();
            //if (strcmp(gpCurrSegment, ".data") != 0)
            //{
            //  gpCurrSegment = ".data";
            //  emit_noindent("\n    .section .data");
            //}
            gEmitToDataSection = 1;
            emit_label(node->flabel);
            emit(".quad %lu", *(uint64_t *)&node->fval);
            gEmitToDataSection = 0;
            //emit_noindent("\n    .section .text");
        }
        emit("movsd %s(#rip), #xmm0", node->flabel);
        break;
    }
    case KIND_ARRAY: {
        if (!node->slabel) {
            node->slabel = make_label();
            //if (strcmp(gpCurrSegment, ".data") != 0)
            //{
            //  gpCurrSegment = ".data";
            //  emit_noindent("\n    .section .data");
           // }
            gEmitToDataSection = 1;
            emit_label(node->slabel);
            emit(".db        \"%s\", 0x00", quote_cstring_len(node->sval, node->ty->size - 1));
            gEmitToDataSection = 0;
            //emit_noindent("\n    .section .text");
        }
        emit("ldx       %s", node->slabel);
        pFrame->ixDestroyed = 1;
        break;
    }
    default:
        error("internal error");
    }
}

/*
==========================================================================================
Split file data into array of lines based on \n  or \r\n endings.
==========================================================================================
*/
static char **split(char *buf) {
    char *p = buf;
    int len = 1;
    while (*p) {
        if (p[0] == '\r' && p[1] == '\n') {
            len++;
            p += 2;
            continue;
        }
        if (p[0] == '\r' || p[0] == '\n')
            len++;
        p++;
    }
    p = buf;
    char **r = malloc(sizeof(char *) * len + 1);
    int i = 0;
    while (*p) {
        if (p[0] == '\r' && p[1] == '\n') {
            p[0] = '\0';
            p += 2;
            r[i++] = p;
            continue;
        }
        if (p[0] == '\r' || p[0] == '\n') {
            p[0] = '\0';
            r[i++] = p + 1;
        }
        p++;
    }
    r[i] = NULL;
    return r;
}

/*
==========================================================================================
Read the source file and split into lines
==========================================================================================
*/
static char **read_source_file(char *file) {
    FILE *fp = fopen(file, "r");
    if (!fp)
        return NULL;
    struct stat st;
    fstat(fileno(fp), &st);
    char *buf = malloc(st.st_size + 1);
    if (fread(buf, 1, st.st_size, fp) != st.st_size) {
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    buf[st.st_size] = '\0';
    return split(buf);
}

/*
==========================================================================================
Test if this line number is a source line, and print it if so
==========================================================================================
*/
static void maybe_print_source_line(char *file, int line)
{
    if (!dumpsource)
        return;
    char **lines = map_get(source_lines, file);
    if (!lines) {
        lines = read_source_file(file);
        if (!lines)
            return;
        map_put(source_lines, file, lines);
    }
    int len = 0;
    for (char **p = lines; *p; p++)
        len++;
    //gMaybeEmitLine = format("# %s", lines[line - 1]);
    if (line > 1)
        emit_nostack("// %s", lines[line - 2]);
}

/*
==========================================================================================
Test if this Node has a file / line mapping and emit it's location if so.
==========================================================================================
*/
static void maybe_print_source_loc(Node *node)
{
    if (!node->sourceLoc)
        return;
    if (node->kind == AST_COMPOUND_STMT)
        return;
    char *file = node->sourceLoc->file;
    long fileno = (long)map_get(source_files, file);
    if (!fileno) {
        fileno = map_len(source_files) + 1;
        map_put(source_files, file, (void *)fileno);
        //gMaybeEmitLoc = format(".file %ld \"%s\"", fileno, quote_cstring(file));
        emit(".file %ld \"%s\"", fileno, quote_cstring(file));
    }
//    if (node->sourceLoc->line == 48)
//        raise(SIGTRAP);
    char *loc = format(".loc %ld %d 0", fileno, node->sourceLoc->line);
    if (strcmp(loc, last_loc)) {
        //gMaybeEmitLoc  = loc;
        emit("%s", loc);
        maybe_print_source_line(file, node->sourceLoc->line);
    }
    last_loc = loc;
}

/*
==========================================================================================
Test if this Node variable has been unassigned ("Unused variable").
==========================================================================================
*/
static void test_unassigned(Node *node)
{
    int idx = find_lvar_offset(node->varname);
    if (idx != -1)
    {
        // Test if variable used before assign
        if (!pFrame->lvars[idx].assigned && !pFrame->lvars[idx].useBeforeAssignWarned)
        {
            char *file = "";
            int lineno = 0;
            if (node->sourceLoc)
            {
                file = node->sourceLoc->file;
                lineno = node->sourceLoc->line;
            }
            printf("%s:%d: Warning: Variable %s may be used without initialization\n", file, lineno,
                    node->varname);
            pFrame->lvars[idx].useBeforeAssignWarned = 1;
        }
        pFrame->lvars[idx].used = 1;
    }
}

/*
==========================================================================================
Generate code to load a local variable value
==========================================================================================
*/
static void emit_lvar(Node *node)
{
    SAVE;
    ensure_lvar_init(node);
    emit_lload(node, node->ty, "sp", node->loff);
    test_unassigned(node);
}

/*
==========================================================================================
Generate code to load a global variable value
==========================================================================================
*/
static void emit_gvar(Node *node)
{
    SAVE;
    emit_gload(node->ty, node->glabel, 0);
}

static void emit_builtin_return_address(Node *node)
{
    emit("// emit_builtin_return_address");
    push("r11");
    assert(vec_len(node->args) == 1);
    emit_expr(vec_head(node->args));
    char *loop = make_label();
    char *end = make_label();
    emit("mov #rbp, #r11");
    emit_label(loop);
    emit("test #rax, #rax");
    emit("jz %s", end);
    emit("mov (#r11), #r11");
    emit("sub $1, #rax");
    emit_jmp(loop);
    emit_label(end);
    emit("mov 8(#r11), #rax");
    pop("r11");
}

// Set the register class for parameter passing to RAX.
// 0 is INTEGER, 1 is SSE, 2 is MEMORY.
static void emit_builtin_reg_class(Node *node)
{
    Node *arg = vec_get(node->args, 0);
    assert(arg->ty->kind == KIND_PTR);
    Type *ty = arg->ty->ptr;
    emit("// emit_builtin_reg_class");
    if (ty->kind == KIND_STRUCT)
        emit("mov $2, #eax");
    else if (is_flotype(ty))
        emit("mov $1, #eax");
    else
        emit("mov $0, #eax");
}

static void emit_builtin_va_start(Node *node)
{
    SAVE;
    assert(vec_len(node->args) == 1);
    emit_expr(vec_head(node->args));
    emit("// emit_builtin_va_start");
    push("rcx");
    emit("movl $%d, (#rax)", numgp * 8);
    emit("movl $%d, 4(#rax)", 48 + numfp * 16);
    emit("lea %d(#rbp), #rcx", -REGAREA_SIZE);
    emit("mov #rcx, 16(#rax)");
    pop("rcx");
}

static bool maybe_emit_builtin(Node *node)
{
    SAVE;
    if (!strcmp("__builtin_return_address", node->fname)) {
        emit_builtin_return_address(node);
        return true;
    }
    if (!strcmp("__builtin_reg_class", node->fname)) {
        emit_builtin_reg_class(node);
        return true;
    }
    if (!strcmp("__builtin_va_start", node->fname)) {
        emit_builtin_va_start(node);
        return true;
    }
    return false;
}

static int get_next_printf_fmt(char *printfFmt, int *pos)
{
    int     x;
    char    fmtChar = 0;

    for (x = *pos; x < strlen(printfFmt); x++)
    {
        // Find first format specifier
        if (printfFmt[x] == '%')
        {
            // Skip conditional formating
            while (printfFmt[x] == '-' || printfFmt[x] == 'u' ||
                   printfFmt[x] == 'l' || printfFmt[x] == '.' ||
                   (printfFmt[x] >= '0' && printfFmt[x] <= '9'))
            {
                x++;
            }
            if (printfFmt[++x] != '%')
            {
                fmtChar = printfFmt[x++];
                break;
            }
        }
    }

    if (printfFmt[x] == 0)
        *pos = x;
    else
        *pos = x+1;

    return fmtChar;
}

/*
==========================================================================================
Generate code to emit function call arguments.
==========================================================================================
*/
static int emit_args(Vector *vals, char *printfFmt, int printfLine)
{
    SAVE;
    int     r = 0;
    char    fmtChar = 0;
    int     pos = 0;
    int     errPos = 0;

    // If this is a printf, parse the format string
//    if (printfFmt)
//        fmtChar = get_next_printf_fmt(printfFmt, &pos);

    for (int i = vec_len(vals) - 1; i >= 0; i--) {
        Node *v = vec_get(vals, i);
        int  kind = v->ty->kind;
        errPos = 0;
        if (kind == KIND_STRUCT)
        {
            emit_addr(v);
            r += push_struct(v->ty->size);
        }
        else if (is_flotype(v->ty))
        {
            emit_expr(v);
            push_xmm(0);
            r += 4;
        }
        else
        {
            emit_expr(v);
            if (v->kind == AST_CONV)
                v = v->operand;

            // Validate printf format character
            if (fmtChar == 's' && !(kind == KIND_PTR || kind == KIND_ARRAY))
            {
                printf("%s:%d:  warning: format '%%s' expects a matching 'char *', type=%d\n",
                        quote_cstring(v->sourceLoc->file), printfLine, kind);
                errPos = pos;
            }
            else if ((fmtChar == 'd' || fmtChar == 'x') && (kind == KIND_PTR ||
                        kind == KIND_ARRAY))
            {
                printf("%s:%d:  warning: format '%%%c' expects a matching 'int', kind=%d\n",
                        quote_cstring(v->sourceLoc->file), printfLine, fmtChar, kind);
                errPos = pos;
            }

            if (errPos)
            {
                char *file = v->sourceLoc->file;
                char **lines = map_get(source_lines, file);
                if (!lines)
                {
                    lines = read_source_file(file);
                    if (!lines)
                        goto no_lines;
                    map_put(source_lines, file, lines);
                }
                int len = 0;
                for (char **p = lines; *p; p++)
                    len++;
                //gMaybeEmitLine = format("# %s", lines[line - 1]);
                printf("%s\n", lines[printfLine - 2]);

                char *pStr = strchr(lines[printfLine - 2], '"');
                int     offset = 0;
                if (pStr)
                    offset = pStr - lines[printfLine - 2];
                for (len = 0; len < offset-1 + pos-1; len++)
                    printf(" ");
                printf("~^\n");
            }
no_lines:

            if (kind == KIND_BOOL ||
                (kind == KIND_CHAR && !printfFmt) ||
                ((kind == KIND_CHAR || kind == KIND_SHORT || kind == KIND_INT) &&
                 fmtChar == 'c'))
            {
                push("a");
                r += 1;
            }
            else if (v->ty->kind == KIND_SHORT || v->ty->kind == KIND_INT ||
                     v->ty->kind == KIND_CHAR)
            {
                if (v->ty->kind == KIND_CHAR)
                    emit_intcast(v->ty);
                emit("stax      0(sp)");
                emit("ads       -2");
                pFrame->stackPos += 2;
                r += 2;
            }
            else if (v->ty->kind == KIND_LONG)
            {
                emit("stax      0(sp)");
                emit("ads       -4");
                pFrame->stackPos += 4;
                r += 4;
            }
            else if (v->ty->kind == KIND_PTR || v->ty->kind == KIND_ARRAY)
            {
                push("ix");
                r += 2;
            }
        }

        // If this is a printf, get next fmtChar
        if (printfFmt)
            fmtChar = get_next_printf_fmt(printfFmt, &pos);
    }
    return r;
}

/*
==========================================================================================
Generate code to convert return value into boolean 1/0
==========================================================================================
*/
static void maybe_booleanize_retval(Type *ty)
{
    if (ty->kind == KIND_BOOL)
    {
        emit("andi      0xFF");
        emit("if        nz");
        emit("ldi       1");
    }
}

static void emit_func_call(Node *node)
{
    SAVE;
    int opos = pFrame->stackPos;
    bool isptr = (node->kind == AST_FUNCPTR_CALL);
    char    *printfFmt = NULL;

//    Type *ftype = isptr ? node->fptr->ty->ptr : node->ftype;

//    Vector *ints = make_vector();
//    Vector *floats = make_vector();
//    Vector *rest = make_vector();

    //int restsize = emit_args(vec_reverse(rest));
    //int restsize = emit_args(rest);
    if (isptr)
    {
        emit_expr(node->fptr);
        push("ix");
        pFrame->stackPos += 2;
    }

    if (!isptr && strcmp(node->fname, "printf") == 0)
    {
        Node *v = vec_get(node->args, 0);
        printfFmt = v->sval;
    }

    int restsize = emit_args(node->args, printfFmt, node->sourceLoc->line);

    // If the function return type is more than one byte, we need
    // stack space for the return value
    if (node->ftype->rettype->size > 1)
        mark_stack_operations(node->ftype->rettype->size);

    if (isptr)
    {
      pop("ix");
      pFrame->stackPos -= 2;
    }
//    if (ftype->hasva)
//        emit("mov $%u, #eax", vec_len(floats));

    if (isptr)
        emit("call_ix");
    else
    {
        int c;
        for (c = 0; c < nLocalFuncs; c++)
            if (strcmp(localFuncs[c], node->fname) == 0)
                break;
        if (c == nLocalFuncs)
            emit_extern(node->fname);
        emit("jal       %s", node->fname);
    }
    maybe_booleanize_retval(node->ty);

    if (restsize > 0)
    {
        // Test if we need to move MSB bytes of the return value
        if (node->ftype->rettype->size > 1)
        {
            emit("swap      1(sp)");
            emit("stax      %d(sp)", 1+restsize);
            emit("swap      1(sp)");
            emit("ads       %d", restsize);
        }
        else 
        {
            emit("ads       %d", restsize);
        }
        pFrame->stackPos -= restsize;
    }

    assert(opos == pFrame->stackPos);
    clear_ix_var();
    clear_acc_var();
}

static void emit_decl(Node *node)
{
    SAVE;
    int idx = find_lvar_offset(node->declvar->varname);
    if (node->sourceLoc)
    {
        if (idx != -1)
        {
            pFrame->lvars[idx].file = node->sourceLoc->file;
            pFrame->lvars[idx].line = node->sourceLoc->line;
        }
    }
    if (!node->declinit)
        return;

    if (idx != -1)
        pFrame->lvars[idx].assigned = 1;
    emit_decl_init(node->declinit, node->declvar->loff, node->declvar->ty->size);
}

static void emit_conv(Node *node)
{
    SAVE;
    // Test for conversion from LITERAL to pointer
    if (node->operand->kind == AST_LITERAL && node->ty->kind == KIND_PTR)
    {
        char *file = node->sourceLoc->file;
        int lineno = node->sourceLoc->line;
        if (node->operand->ival < 0)
            printf("%s:%d: Warning: cast of negative number to PTR\n", file, lineno);
        if (node->operand->ival > 32767)
            printf("%s:%d: Warning: cast to PTR truncated\n", file, lineno);
        emit("ldx       %d", node->operand->ival);
        clear_ix_var();
        pFrame->ixDestroyed = 1;
        return;
    }
    emit_expr(node->operand);
    emit_load_convert(node->ty, node->operand->ty);
}

static void emit_deref(Node *node)
{
    SAVE;
    printf("Emit DEFEF\n");
    emit_expr(node->operand);
    emit_lload(node, node->operand->ty->ptr, "ix", 0);
    emit_load_convert(node->ty, node->operand->ty->ptr);
}

static void emit_ternary(Node *node)
{
    SAVE;
    pFrame->isTernary++;

    // Test for "if (a)" syntax where a is 2-byte LVAR
    if (node->cond->kind == AST_LVAR &&
        node->cond->ty->size == 2)
    {
        pFrame->emitCompZero = 1;
    }

    emit_expr(node->cond);
    pFrame->emitCompZero = 0;

    // Very special case
    if (node->then && node->els &&
        node->then->kind == AST_FUNCALL &&
        node->els->kind == AST_FUNCALL &&
        node->then->ty->kind == KIND_VOID && 
        node->els->ty->kind == KIND_VOID && 
        vec_len(node->then->args) == 0 &&
        vec_len(node->els->args) == 0)
    {
        emit("ifte      z");
        emit_expr(node->els);
        emit_expr(node->then);
        pFrame->isTernary--;
        return;
    }

    char *ne = make_label();

    switch (node->cond->kind) {
    case '<':
    case '>':
    case OP_EQ:
    case OP_LE:
    case OP_NE:
    case OP_GE:
        break;
    case '&':
        if (node->cond->left->ty->size > 1 && node->cond->right->ty->size > 1)
            emit("or        1(sp)");
        emit_je(ne);
        break;
    case '|':
        if (node->cond->ty->size > 1)
            emit("or        1(sp)");
        emit_je(ne);
        break;
    default:
        if (node->cond->kind == AST_FUNCALL)
        {
            if (node->cond->ty->size > 1)
                emit("or        1(sp)");
            else
                emit("cpi       0");
        }
        emit_je(ne);
    }

    emit_jmp(ne);

    if (node->then)
        emit_expr(node->then);
    if (node->els) {
        clear_acc_var();
        clear_ix_var();
        char *end = make_label();
        if (!node->then || node->then->kind != AST_GOTO)
            emit_jmp(end);
        pFrame->preserveVars = 1;
        emit_label(ne);
        emit_expr(node->els);
        if (!node->then || node->then->kind != AST_GOTO)
            emit_label(end);
    } else {
        emit_label(ne);
    }

    pFrame->isTernary--;

    // We don't know if the then or else path is taken, so we 
    // won't know the value of acc or ix at the end
    clear_ix_var();
    clear_acc_var();
}

/*
==========================================================================================
Emit a GOTO
==========================================================================================
*/
static void emit_goto(Node *node)
{
    SAVE;
    char        label[128];
    int         labelFound = 0;
    asm_line_t  *pLine;

    assert(node->newlabel);
    emit_jmp(node->newlabel);
    if (!pFrame->isTernary && pFrame->lastSwapOptional)
    {
        // Test if the jump location performs a load of ACC prior to use
        strcpy(label, &pFrame->pAsmLines->pPrev->pLine[14]);
        strcat(label, ":");
        pLine = pFrame->pAsmLines->pNext;
        while (pLine != pFrame->pAsmLines)
        {
            // IF we have found the label, then search for the first line
            // that either uses or changes Acc
            if (labelFound)
            {
                if (isAccLoad(&pLine->pLine[4]))
                {
                    // We can delete the optional swap
                    delete_asm_line(pFrame->pLastSwapLine);
                    pFrame->pLastSwapLine = NULL;
                    pFrame->lastSwapOptional = 0;
                    break;
                }
                else if (isAccUse(&pLine->pLine[4]))
                {
                    // Last optional swap needed
                    pFrame->pLastSwapLine = NULL;
                    pFrame->lastSwapOptional = 0;
                    break;
                }
            }
            else
            {
                if (strcmp(label, pLine->pLine) == 0)
                    labelFound = 1;
            }

            pLine = pLine->pNext;
        }
    }
}

static void emit_return(Node *node)
{
    SAVE;
    if (node->retval)
    {
        emit_expr(node->retval);
        maybe_booleanize_retval(node->retval->ty);
    }
    emit("jal       _L%s_ret", pFrame->fname);
    pFrame->retCount++;
}

static void emit_compound_stmt(Node *node)
{
    SAVE;
    for (int i = 0; i < vec_len(node->stmts); i++)
        emit_expr(vec_get(node->stmts, i));
}

/*
==========================================================================================
Reverse the if condition of the given line
==========================================================================================
*/
static void reverse_if_comparison(asm_line_t *pLine)
{
    char        newComp[8];
    char        newline[32];

    // Validate the last asm line was if
    if (strncmp(&pLine->pLine[4], "if", 2) != 0)
        return;

    // Test for eq
    if (strcmp(&pLine->pLine[14], "eq") == 0)
        strcpy(newComp, "ne");
    else if (strcmp(&pLine->pLine[14], "ne") == 0)
        strcpy(newComp, "eq");
    else if (strcmp(&pLine->pLine[14], "nz") == 0)
        strcpy(newComp, "z");
    else if (strcmp(&pLine->pLine[14], "z") == 0)
        strcpy(newComp, "nz");
    else if (strcmp(&pLine->pLine[14], "lt") == 0)
        strcpy(newComp, "ge");
    else if (strcmp(&pLine->pLine[14], "gt") == 0)
        strcpy(newComp, "le");
    else if (strcmp(&pLine->pLine[14], "ge") == 0)
        strcpy(newComp, "lt");
    else if (strcmp(&pLine->pLine[14], "le") == 0)
        strcpy(newComp, "gt");
    else if (strcmp(&pLine->pLine[14], "slt") == 0)
        strcpy(newComp, "sge");
    else if (strcmp(&pLine->pLine[14], "sgt") == 0)
        strcpy(newComp, "sle");
    else if (strcmp(&pLine->pLine[14], "sge") == 0)
        strcpy(newComp, "slt");
    else if (strcmp(&pLine->pLine[14], "sle") == 0)
        strcpy(newComp, "sgt");

    pLine->pLine[14] = 0;
    sprintf(newline, "%s%s", pLine->pLine, newComp);
    free(pLine->pLine);
    pLine->pLine = strdup(newline);
}

/*
==========================================================================================
Reverse the condition of the last emitted asm line, which should be an 'if'
==========================================================================================
*/
static void reverse_last_if_comparison(void)
{
    asm_line_t  *pLine;

    pLine = get_last_asm_line();

    reverse_if_comparison(pLine);
}

static int convert_if_to_iftt(asm_line_t *pLine)
{
    // Validate the last asm line was if
    if (strncmp(&pLine->pLine[4], "if", 2) != 0)
        return 0;

    pLine->pLine[6] = 't';
    pLine->pLine[7] = 't';
    return 1;
}

static int convert_if_to_ifte(void)
{
    asm_line_t  *pLine;

    pLine = get_last_asm_line();

    // Validate the last asm line was if
    if (strncmp(&pLine->pLine[4], "if", 2) != 0)
        return 0;

    pLine->pLine[6] = 't';
    pLine->pLine[7] = 'e';
    return 1;
}

/*
==========================================================================================
Emit a logical AND
==========================================================================================
*/
static void emit_logand(Node *node)
{
    SAVE;
    char *end = make_label();
    int  kind = node->left->kind;;
    asm_line_t *pLine;

    emit_expr(node->left);
    maybe_emit_bool_compare(node->left);
    pLine = get_last_asm_line();
    if (strcmp(&pLine->pLine[4], "if        z") != 0)
    {
        if (convert_if_to_iftt(pLine))
            emit("ldz       1");
    }
    if (kind == OP_LOGAND || kind == OP_LOGOR || kind == '!')
        emit("if        z");
    emit_jmp(end);
    emit_expr(node->right);
    maybe_emit_bool_compare(node->right);

    pLine = get_last_asm_line();
    if (strcmp(&pLine->pLine[4], "if        z") == 0 ||
        strcmp(&pLine->pLine[4], "if        eq") == 0)
    {
        delete_asm_line(pLine);
    }
    else
    {
        if (convert_if_to_ifte())
        {
            emit("ldz       1");
            emit("ldz       0");
        }
    }
    emit_label(end);
}

/*
==========================================================================================
Emit a logical OR
==========================================================================================
*/
static void emit_logor(Node *node)
{
    SAVE;
    char *end = make_label();
    int  kind = node->left->kind;;
    asm_line_t  *pLine;
    char  *comp;
    emit_expr(node->left);
    maybe_emit_bool_compare(node->left);

    if (kind == OP_LOGAND || kind == OP_LOGOR || kind == '!')
    {
        emit("if        nz");
        emit_jmp(end);
    }
    else
    {
        // Reverse the comparison of the last if .. we want the
        // TRUE version
        reverse_last_if_comparison();
        
        // If the last comparison had an EQUAL, then we need to load z with 0
        // and convert the "if" to "iftt"
        pLine = get_last_asm_line();
        comp = &pLine->pLine[14];
        if (strcmp(comp, "z")  == 0 || strcmp(comp, "eq") == 0 ||
            strstr(comp, "ge") != NULL || strstr(comp, "le") != NULL)
        {
            // Convert the IF to IFTT
            convert_if_to_iftt(pLine);
            emit("ldz       0");        // Emit a TRUE comparison (i.e. Not Zero)
        }

        emit_jmp(end);
    }

    emit_expr(node->right);
    maybe_emit_bool_compare(node->right);

    // If the last comparison had an EQUAL, then we need to load Z with 0
    // so the statement evaluates TRUE
    pLine = get_last_asm_line();
    if (strcmp(&pLine->pLine[4], "if        z") == 0 ||
        strcmp(&pLine->pLine[4], "if        eq") == 0)
    {
        delete_asm_line(pLine);
    }
    else
    {
        if (convert_if_to_ifte())
        {
            emit("ldz       1");
            emit("ldz       0");
        }
    }
    emit_label(end);
}

/*
==========================================================================================
Emit a logical NOT
==========================================================================================
*/
static void emit_lognot(Node *node)
{
    SAVE;
    emit_expr(node->operand);
    emit("ldz       ~z");
}

/*
==========================================================================================
Emit a bitwise AND
==========================================================================================
*/
static void emit_bitand(Node *node)
{
    SAVE;
    emit_expr(node->left);
    if (node->right->kind == AST_LITERAL && node->right->ty->kind == KIND_CHAR)
    {
        emit("andi      %d", node->right->ival);
    }
    else
    {
        if (node->right->ty->size == 1)
        {
            mark_stack_operations(1);
            emit("stax      0(sp)");
            emit_expr(node->right);
            emit("and       0(sp)");
        }
        else
        {
            // Two-byte rigth operand.  Save LEFT at 2/3
            emit("stax      0(sp)");
            emit("ads       -2");
            pFrame->stackPos += 2;
            emit_expr(node->right);

            // Test if both left and right are 2 byte
            if (node->left->ty->size == 1)
            {
                // Test if we can easily remove loading of the MSB
                asm_line_t *pL1 = get_last_asm_line();
                if (strcmp(pL1->pPrev->pLine, "    stax      1(sp)") == 0 &&
                    strncmp(pL1->pPrev->pPrev->pLine, "    ldax", 8) == 0)
                {
                    delete_asm_line(pL1->pPrev->pPrev);
                    delete_asm_line(pL1->pPrev);
                }
                // We only need the LSB
                emit("and       2(sp)"); // AND LSBs
            }
            else
            {
                emit("and       2(sp)"); // AND LSBs
                emit("stax      2(sp)"); // Save LSBs
                emit("ldax      1(sp)"); // Get MSB of right-hand side
                emit("and       3(sp)"); // AND MSBs
                emit("stax      3(sp)"); // Save MSB
                emit("ldax      2(sp)"); // Get LSB
            }
            emit("ads       2");
            pFrame->stackPos -= 2;
            mark_stack_operations(2);
        }
    }
}

/*
==========================================================================================
Generate code for OR and XOR
==========================================================================================
*/
static void emit_bitor_xor(Node *node)
{
    int     kind;
    char    *op = node->kind == '|' ? "or" : "xor";
    char    varAddr[256];
    Node    *l = node->left;
    Node    *r = node->right;

    SAVE;

    // If one side is access or sfr, ensure it is on the right
    if (node->ty->size == 1 && ((l->ty->isaccess ^ r->ty->isaccess) == 1 ||
        (l->ty->issfr ^ r->ty->issfr) == 1) && !(r->kind == AST_LITERAL))
    {
        if (l->ty->issfr || l->ty->isaccess)
        {
            // Swap the nodes
            node->left = r;
            node->right = l;
        }
    }

    l = node->left;
    r = node->right;

    // Test for simple 8-bit literal case
    if ((node->ty->kind == KIND_CHAR || node->ty->kind == KIND_BOOL) &&
         (r->kind == AST_LITERAL || r->kind == AST_LVAR || r->kind == AST_GVAR) &&
         (l->kind == AST_LVAR || (l->kind == AST_GVAR && !l->ty->issfr && !l->ty->isaccess)))
    {
        emit_expr(r);
        //emit("ldi       %d", node->right->ival & 0xFF);
        if (node->left->kind == AST_LVAR)
        {
            // Test if node has been assigned
            test_unassigned(node->left);
            if (node->left->ty->isparam)
                emit("%-3s       $%d(sp)", op, node->left->loff + pFrame->stackPos);
            else
                emit("%-3s       %d(sp)", op, node->left->loff + pFrame->stackPos);
            pFrame->pAsmLines->pPrev->stackRelative = 1;
        }
        else
        {
            sprintf(varAddr, "&%s", node->left->varname);
            if (strcmp(varAddr, pFrame->ixVar) != 0)        
            {
                emit("ldx       %s", node->left->varname);
                set_ix_var(varAddr);
            }
            emit("%-3s       0(ix)", op);
        }
        pFrame->accVal = -1000;

        return;
    }
    
    emit_expr(node->left);

    // Test for right hand AST_LITERAL, AST_LVAR or AST_GVAR
    kind = node->right->kind;
    if (kind == AST_LITERAL || kind == AST_LVAR || kind == AST_GVAR)
    {
        if (kind == AST_LITERAL)
        {
            emit("stax      0(sp)");
            emit("ldi       %d", node->right->ival & 0xFF);
            emit("%-3s       0(sp)", op);
            mark_stack_operations(1);
            if (node->ty->kind == KIND_SHORT || node->ty->kind == KIND_INT)
            {
                emit("ldi       %d", (node->right->ival >> 8) & 0xFF);
                emit("%-3s       1(sp)", op);
                emit("stax      1(sp)");
                emit("ldax      0(sp)");
            }
        }
        else if (kind == AST_LVAR)
        {
            if (node->right->ty->isparam)
                emit("%-3s       $%d(sp)", op, node->right->loff + pFrame->stackPos);
            else
                emit("%-3s       %d(sp)", op, node->right->loff + pFrame->stackPos);
            pFrame->pAsmLines->pPrev->stackRelative = 1;
            if (node->ty->kind == KIND_SHORT || node->ty->kind == KIND_INT)
            {
                emit("swap      1(sp)");
                if (node->right->ty->isparam)
                    emit("%-3s       $%d(sp)", op, node->right->loff + pFrame->stackPos+1);
                else
                    emit("%-3s       %d(sp)", op, node->right->loff + pFrame->stackPos+1);
                pFrame->pAsmLines->pPrev->stackRelative = 1;
                emit("swap      1(sp)");
            }
        }
        else
        {
            if (node->right->ty->issfr)
            {
                emit("stax      0(sp)");
                emit("lda       %s", node->right->varname);
                emit("%-3s       0(sp)", op);
                mark_stack_operations(1);
            }
            else
            {
                sprintf(varAddr, "&%s", node->left->varname);
                if (strcmp(varAddr, pFrame->ixVar) != 0)        
                {
                    emit("ldx       %d", node->right->varname);
                    set_ix_var(varAddr);
                    pFrame->ixModified = 1;
                }
                emit("%-3s       0(ix)", op);
                if (node->ty->kind == KIND_SHORT || node->ty->kind == KIND_INT)
                {
                    emit("stax      0(sp)");
                    emit("ldax      1(ix)");
                    emit("%-3s       1(sp)", op);
                    emit("stax      1(sp)");
                    emit("ldax      0(sp)");
                }
            }
        }
    }
    else
    {
        // Save LSB to stack and advance by 2
        emit("stax      0(sp)");
        emit("ads       -2");
        pFrame->stackPos += 2;
        emit_expr(node->right);
        emit("%-3s       2(sp)", op);
        emit("sta       2(sp)");
        if (node->ty->kind == KIND_SHORT || node->ty->kind == KIND_INT)
        {
            emit("lda       1(sp)");
            emit("%-3s       3(sp)", op);
            emit("sta       3(sp)");
            emit("lda       2(sp)");
        }
        emit("ads       2");
        pFrame->stackPos -= 2;
    }
    pFrame->accVal = -1000;
}

/*
==========================================================================================
Generate code for bitwise NOT operation
==========================================================================================
*/
static void emit_bitnot(Node *node)
{
    SAVE;
    emit_expr(node->left);
    if (node->ty->kind == KIND_CHAR)
    {
        emit("stax      0(sp)");
        emit("ldi       0xFF");
        emit("xor       0(sp)");
        mark_stack_operations(1);
    }
    else
    {
        emit("non-char bitnot support needed!!");
    }
}

/*
==========================================================================================
Emit operand and convert to CAST type
==========================================================================================
*/
static void emit_cast(Node *node)
{
    SAVE;
    // Test for cast from AST_LITERAL to KIND_PTR
    if (node->operand->kind == AST_LITERAL && node->ty->kind == KIND_PTR)
    {
        char *file = node->sourceLoc->file;
        int lineno = node->sourceLoc->line;
        if (node->operand->ival < 0)
            printf("%s:%d: Warning: cast of negative number to PTR\n", file, lineno);
        if (node->operand->ival > 32767)
            printf("%s:%d: Warning: cast to PTR truncated\n", file, lineno);
        emit("ldx       %d", node->operand->ival);
        clear_ix_var();
        pFrame->ixDestroyed = 1;
        return;
    }
    emit_expr(node->operand);
    emit_load_convert(node->ty, node->operand->ty);
    return;
}

static void emit_comma(Node *node)
{
    SAVE;
    emit_expr(node->left);
    emit_expr(node->right);
}

/*
==========================================================================================
Generate code to assign to a variable
==========================================================================================
*/
static void emit_assign(Node *node)
{
    SAVE;
    char    varAddr[256];

    asm_line_t  *pLine;
    if (node->left->ty->kind == KIND_STRUCT &&
        node->left->ty->size > 8)
    {
        emit_copy_struct(node->left, node->right);
    }
    else
    {
        // Test for simple assignment
        if (node->right->kind == AST_LITERAL &&
            node->right->ty->kind == KIND_CHAR)
        {
            // Test for literal assign to struct that is a bitfield
            if (!(node->left->kind == AST_STRUCT_REF && node->left->ty->bitsize > 0))
                // Get the literal
                emit_expr(node->right);

            // Test for SFR
            if (node->left->ty->issfr)
                emit("sta       %s", node->left->varname);
            // Test for LVAR
            else if (node->left->kind == AST_LVAR)
            {
                emit("stax      %s%d(sp)", node->left->ty->isparam ? "$" : "",
                    node->left->loff + pFrame->stackPos);
                set_acc_var(node->left->varname);
                pFrame->pAsmLines->pPrev->stackRelative = 1;
            }
            else if (node->left->kind == AST_GVAR)
            {
                if (node->left->ty->isaccess)
                {
                    // Access directly
                    emit("sta       %s", node->left->glabel);
                    set_acc_var(node->left->varname);
                }
                else
                {
                    // Access via ix
                    sprintf(varAddr, "&%s", node->left->glabel);
                    if (strcmp(varAddr, pFrame->ixVar) != 0)        
                    {
                        emit("ldx       %s", node->left->glabel);
                        set_ix_var(varAddr);
                    }
                    emit("stax      0(ix)");
                    pFrame->ixDestroyed = 1;
                    set_acc_var(node->left->glabel);
                }
            }
            else if (node->left->kind == AST_DEREF)
            {
                if (node->ty->kind != node->right->ty->kind)
                    emit_load_convert(node->ty, node->right->ty);
                emit_assign_deref(node->left, NULL);
            }
            else
            {
                // Test for literal assign to struct that is a bitfield
                if (node->left->kind == AST_STRUCT_REF && node->left->ty->bitsize > 0)
                {
                    // Change the literal value to be ANDED and bit shifted to
                    // match the struct field location
                    node->right->ival = (node->right->ival & ((1 << node->left->ty->bitsize)-1))
                                        << node->left->ty->bitoff;
                    emit_expr(node->right);
                    emit("stax      0(sp)");
                    mark_stack_operations(1);
                    pFrame->accOnStack = 1;
                    pFrame->noBitShiftStruc = 1;
                    emit_store(node->left, NULL);
                    pFrame->noBitShiftStruc = 0;
                }
                else
                {
                    if (node->ty->kind != node->right->ty->kind)
                        emit_load_convert(node->ty, node->right->ty);
                    emit_store(node->left, NULL);
                }
            }
        }
        else
        {
            Node *right = node->right;
            while (right->kind == AST_CONV)
                right = right->operand;

            // Test for binop on right hand side
            switch (right->kind)
            {
            case '<':
            case '>':
            case OP_GE:
            case OP_LE:
            case OP_EQ:
            case OP_NE:
                // The last opcode was an 'if' statement. We must convert
                // to a BOOL type
                emit_expr(node->right);
                pLine = get_last_asm_line();
                if (strncmp(&pLine->pLine[4], "if", 2) != 0)
                {
                    emit("ldaz      z");
                }
                else
                {
                    reverse_last_if_comparison();

                    // Convert the if to an ldac
                    pLine->pLine[4] = 'l';
                    pLine->pLine[5] = 'd';
                    pLine->pLine[6] = 'a';
                    pLine->pLine[7] = 'c';
                }

                if (node->ty->kind != node->right->ty->kind)
                    emit_load_convert(node->ty, node->right->ty);
                emit_store(node->left, NULL);
                break;

            default:
                // Test for load of pointer
                if (node->left->kind == AST_LVAR && node->left->ty->kind == KIND_PTR &&
                    node->right->ty->size == 2 && node->right->kind == AST_LVAR)
                {
                    emit("ldxx      %s%d(sp)", node->right->ty->isparam ? "$":"",
                        node->right->loff + pFrame->stackPos);
                    pFrame->pAsmLines->pPrev->stackRelative = 1;
                    set_ix_var(node->left->varname);
                    emit("stxx      %s%d(sp)", node->left->ty->isparam ? "$":"",
                        node->left->loff + pFrame->stackPos);
                    pFrame->pAsmLines->pPrev->stackRelative = 1;
                }
                else
                {
                    // Test for deref load of AST_LITERAL
                    if (node->right->kind == AST_LITERAL &&
                        node->left->kind == AST_DEREF)
                    {
                        // Mark variable as being assigned
                        char *varname = "";
                        if (node->left->operand->kind == AST_CONV)
                            varname = node->left->operand->operand->varname;
                        else if (node->left->operand->kind == '+')
                        {
                            if (node->left->operand->left->kind == AST_CONV)
                                varname = node->left->operand->left->operand->varname;
                            else
                                varname = node->left->operand->left->varname;
                        }
                        int idx = find_lvar_offset(varname);
                        if (idx != -1)
                            pFrame->lvars[idx].assigned = 1;

                        // Emit the expression
                        emit_expr(node->left->operand);
                        emit("ldi       %d", (node->right->ival >> 8) & 0xFF);
                        emit("stax      %d(ix)", node->left->operand->ty->offset+1);
                        emit("ldi       %d", node->right->ival & 0xFF);
                        emit("stax      %d(ix)", node->left->operand->ty->offset);
                    }
                    else
                    {
                        Node *simpleLoad = NULL;
                        if (node->right->kind == AST_LVAR)
                            simpleLoad = node->right;
                        else
                        {
                            emit_expr(node->right);
                            if (node->left->kind == AST_DEREF ||
                                node->left->kind == AST_STRUCT_REF)
                            {
                                emit("stax      0(sp)");
                                mark_stack_operations(1);
                                pFrame->accOnStack = 1;
                            }
                        }
                        if (node->ty->kind != node->right->ty->kind)
                            emit_load_convert(node->ty, node->right->ty);
                        emit_store(node->left, simpleLoad);
                    }
                }
            }
        }
    }

    int idx = find_lvar_offset(node->left->varname);
    if (idx != -1)
        pFrame->lvars[idx].assigned = 1;
}

/*
==========================================================================================
Emit Label Address
==========================================================================================
*/
static void emit_label_addr(Node *node)
{
    SAVE;
    char    varAddr[256];
    sprintf(varAddr, "&%s", node->left->varname);
    if (strcmp(varAddr, pFrame->ixVar) != 0)        
    {
        emit("ldx       %s", node->newlabel);
        set_ix_var(varAddr);
        pFrame->ixDestroyed = 1;
    }
}

/*
==========================================================================================
Emit JMP_IX (computed GOTO)
==========================================================================================
*/
static void emit_computed_goto(Node *node)
{
    SAVE;
    emit_expr(node->operand);
    emit("jmp_ix");
}

/*
==========================================================================================
Emit Expression
==========================================================================================
*/
static int emit_expr(Node *node)
{
    SAVE;
    if (node->kind != AST_GVAR && node->kind != AST_LVAR)
        maybe_print_source_loc(node);
    switch (node->kind) {
    case AST_LITERAL: emit_literal(node); return 0;
    case AST_LVAR:    emit_lvar(node); return 0;
    case AST_GVAR:    emit_gvar(node); return 0;
    case AST_FUNCDESG: emit_addr(node); return 0;
    case AST_FUNCALL:
        if (maybe_emit_builtin(node))
            return 0;
        // fall through
    case AST_FUNCPTR_CALL:
        emit_func_call(node);
        return 0;
    case AST_DECL:    emit_decl(node); return 0;
    case AST_CONV:    emit_conv(node); return 0;
    case AST_ADDR:    emit_addr(node->operand); return 0;
    case AST_DEREF:   emit_deref(node); return 0;
    case AST_IF:
    case AST_TERNARY:
        emit_ternary(node);
        return 0;
    case AST_GOTO:    emit_goto(node); return 0;
    case AST_LABEL:
        if (node->newlabel)
            emit_label(node->newlabel);
        return 0;
    case AST_RETURN:  emit_return(node); return 0;
    case AST_COMPOUND_STMT: emit_compound_stmt(node); return 0;
    case AST_STRUCT_REF:
        emit_load_struct_ref(node, node->struc, node->ty, 0);
        return 0;
    case OP_PRE_INC:   emit_pre_inc_dec(node, "add"); return 0;
    case OP_PRE_DEC:   emit_pre_inc_dec(node, "sub"); return 0;
    case OP_POST_INC:  emit_post_inc_dec(node, "add"); return 0;
    case OP_POST_DEC:  emit_post_inc_dec(node, "sub"); return 0;
    case '!': emit_lognot(node); return 0;
    case '&': emit_bitand(node); return 0;
    case '|': emit_bitor_xor(node); return 0;
    case '^': emit_bitor_xor(node); return 0;
    case '~': emit_bitnot(node); return 0;
    case OP_LOGAND: emit_logand(node); return 0;
    case OP_LOGOR:  emit_logor(node); return 0;
    case OP_CAST:   emit_cast(node); return 0;
    case ',': emit_comma(node); return 0;
    case '=': emit_assign(node); return 0;
    case OP_LABEL_ADDR: emit_label_addr(node); return 0;
    case AST_COMPUTED_GOTO: emit_computed_goto(node); return 0;
    case AST_PRUNED: return 0;
    default:
        return emit_binop(node);
    }

    return 0;
}

static void emit_zero(int size)
{
    SAVE;
    for (; size >= 8; size -= 8) emit(".quad 0");
    for (; size >= 4; size -= 4) emit(".long 0");
    for (; size > 0; size--)     emit(".db 0");
}

static void emit_padding(Node *node, int off)
{
    SAVE;
    int diff = node->initoff - off;
    assert(diff >= 0);
    emit_zero(diff);
}

static void emit_data_addr(Node *operand, int depth)
{
    switch (operand->kind) {
    case AST_LVAR:
    {
        char *label = make_label();
        emit(".data %d", depth + 1);
        emit_label(label);
        do_emit_data(operand->lvarinit, operand->ty->size, 0, depth + 1);
        emit(".data %d", depth);
        emit(".quad %s", label);
        return;
    }
    case AST_GVAR:
        emit(".quad %s", operand->glabel);
        return;
    default:
        error("internal error");
    }
}

static void emit_data_charptr(char *s, int depth)
{
    char *label = make_label();
    emit(".data      %d", depth + 1);
    emit_label(label);
    emit(".db        \"%s\"", quote_cstring(s));
    emit(".data      %d", depth);
    emit(".quad      %s", label);
}

static void emit_data_primtype(Type *ty, Node *val, int depth)
{
    switch (ty->kind) {
    case KIND_FLOAT:
    {
        float f = val->fval;
        emit(".long %d", *(uint32_t *)&f);
        break;
    }
    case KIND_DOUBLE:
        emit(".quad %ld", *(uint64_t *)&val->fval);
        break;
    case KIND_BOOL:
        emit(".db %d", !!eval_intexpr(val, NULL));
        break;
    case KIND_CHAR:
        emit(".db %d", eval_intexpr(val, NULL));
        break;
    case KIND_SHORT:
        emit(".dw %d", eval_intexpr(val, NULL));
        break;
    case KIND_INT:
        emit(".dw %d", eval_intexpr(val, NULL));
        break;
    case KIND_LONG:
    case KIND_LLONG:
    case KIND_PTR:
        if (val->kind == OP_LABEL_ADDR)
        {
            emit(".quad %s", val->newlabel);
            break;
        }
        bool is_char_ptr = (val->operand->ty->kind == KIND_ARRAY && val->operand->ty->ptr->kind == KIND_CHAR);
        if (is_char_ptr)
            emit_data_charptr(val->operand->sval, depth);
        else if (val->kind == AST_GVAR)
            emit(".quad %s", val->glabel);
        else
        {
            Node *base = NULL;
            int v = eval_intexpr(val, &base);
            if (base == NULL)
            {
                emit(".quad %u", v);
                break;
            }
            Type *ty = base->ty;
            if (base->kind == AST_CONV || base->kind == AST_ADDR)
                base = base->operand;
            if (base->kind != AST_GVAR)
                error("global variable expected, but got %s", node2s(base, 0));
            assert(ty->ptr);
            emit(".quad %s+%u", base->glabel, v * ty->ptr->size);
        }
        break;
    default:
        error("don't know how to handle\n  <%s>\n  <%s>", ty2s(ty), node2s(val, 0));
    }
}

static void do_emit_data(Vector *inits, int size, int off, int depth)
{
    SAVE;
    for (int i = 0; i < vec_len(inits) && 0 < size; i++)
    {
        Node *node = vec_get(inits, i);
        Node *v = node->initval;
        emit_padding(node, off);
        if (node->totype->bitsize > 0)
        {
            assert(node->totype->bitoff == 0);
            long data = eval_intexpr(v, NULL);
            Type *totype = node->totype;
            for (i++ ; i < vec_len(inits); i++)
            {
                node = vec_get(inits, i);
                if (node->totype->bitsize <= 0)
                    break;
                v = node->initval;
                totype = node->totype;
                data |= ((((long)1 << totype->bitsize) - 1) & eval_intexpr(v, NULL)) << totype->bitoff;
            }
            emit_data_primtype(totype, &(Node){ AST_LITERAL, totype, .ival = data }, depth);
            off += totype->size;
            size -= totype->size;
            if (i == vec_len(inits))
                break;
        }
        else
        {
            off += node->totype->size;
            size -= node->totype->size;
        }
        if (v->kind == AST_ADDR)
        {
            emit_data_addr(v->operand, depth);
            continue;
        }
        if (v->kind == AST_LVAR && v->lvarinit)
        {
            do_emit_data(v->lvarinit, v->ty->size, 0, depth);
            continue;
        }
        emit_data_primtype(node->totype, node->initval, depth);
    }
    emit_zero(size);
}

static void emit_data(Node *v, int off, int depth)
{
    SAVE;
    if (strcmp(gpCurrSegment, ".data") != 0)
    {
      gpCurrSegment = ".data";
      emit(".section .data");
    }
    gEmitToDataSection = 1;
    if (!v->declvar->ty->isstatic)
        emit(".public %s", v->declvar->glabel);
    else
        emit(".local %s", v->declvar->glabel);
    emit_noindent("%s:", v->declvar->glabel);
    do_emit_data(v->declinit, v->declvar->ty->size, off, depth);
    gEmitToDataSection = 0;
}

static void emit_bss(Node *v)
{
    char  label[1024];

    SAVE;
    if (strcmp(gpCurrSegment, ".bss") != 0)
    {
      gpCurrSegment = ".bss";
      emit(".section .bss");
    }
    if (!v->declvar->ty->isstatic)
        emit(".public %s", v->declvar->glabel);
    else
        emit(".local %s", v->declvar->glabel);
    sprintf(label, "%s:", v->declvar->glabel);
    emit_noindent("%-30s", label);
    emit(".ds      %d", v->declvar->ty->size);
}

static void emit_global_var(Node *v)
{
    SAVE;
    if (v->declinit)
    {
        // Test for SFR declaration
        if (v->declvar->ty && v->declvar->ty->issfr)
        {
            int  val;

            Node *init = vec_get(v->declinit, 0);
            if (v->declvar->ty->kind == KIND_STRUCT)
                val = init->initval->struc->ival;
            else
                val = init->initval->ival;
            emit_noindent("##define  %-20s 0x%02X", v->declvar->glabel, val | 0x200);
            //emit_noindent("#define  %s %d", v->declvar->glabel, v->initval);
        }
        else
           emit_data(v, 0, 0);
    }
    else
        emit_bss(v);
}

/*
==========================================================================================
Calculate the stack offset of function parameters.
==========================================================================================
*/
static void calc_func_params(Vector *params)
{
    int xreg = 0;
    int arg = 2;
    int off = 0;

    // Loop for all parameters
    pFrame->nparam = vec_len(params);
    for (int i = 0; i < vec_len(params); i++)
    {
        Node *v = vec_get(params, i);
        v->ty->isparam = true;
        if (v->ty->kind == KIND_STRUCT)
        {
            emit("lea %d(#rbp), #rax", arg * 8);
            int size = push_struct(v->ty->size);
            off -= size;
            arg += size / 8;
        }
        else if (is_flotype(v->ty))
        {
            if (xreg >= 8)
            {
                emit("mov %d(#rbp), #rax", arg++ * 8);
                push("rax");
            }
            else
                push_xmm(xreg++);
            off -= 8;
        }
        else
        {
          pFrame->param[i].name = v->varname;
          pFrame->param[i].kind = v->ty->kind;
          pFrame->param[i].size = v->ty->size;
          pFrame->param[i].stackPos = off;
          v->loff = off;
          v->isParam = 1;
          off += v->ty->size;
        }
    }
}

/*
==========================================================================================
Tests if ra changed in the routine and adds sra / lra plus updates all offsets to
stack paramters.
==========================================================================================
*/
static void adjust_stack_for_ra_change(void)
{
    asm_line_t  *pLine;
    asm_line_t  *pRef;
    char        str[128];

    // Test if RA was changed in the routine
    if (pFrame->raDestroyed && pFrame->retCount > 0)
    {
        // We need to make the adjustment.  Add an "sra" instruction prior to the
        // "ads" line.
        pLine = (asm_line_t *) malloc(sizeof(asm_line_t));
        pLine->pLine = strdup("    sra");

        // Find the "ads" line
        pRef = pFrame->pAsmLines;
        while (pRef->pNext != pFrame->pAsmLines)
        {
            if (strstr(pRef->pLine, "    ads  ") != NULL)
                break;
            pRef = pRef->pNext;
        }

        insert_asm_line_before(pLine, pRef);
        
        // Add an 'lra' instruction at the end
        pRef = pFrame->pAsmLines->pPrev;
        while (pRef != pFrame->pAsmLines && strncmp(pRef->pLine, "    ret", 8) != 0)
            pRef = pRef->pPrev;
        if (pRef != pFrame->pAsmLines)
        {
            pLine = (asm_line_t *) malloc(sizeof(asm_line_t));
            pLine->pLine = strdup("    lra");

            insert_asm_line_before(pLine, pRef);
        }
        
        // Scan all lines and add 1 to any stack relative params (i.e. "%d(sp)")
        pLine = pFrame->pAsmLines->pNext;
        while (pLine != pFrame->pAsmLines)
        {
            if (pLine->pLine[14] == '$')
            {
                int     off = atoi(&pLine->pLine[15]);
              
                strncpy(str, pLine->pLine, 14);
                str[14] = 0;
                sprintf(&str[14], "$%d(sp)", off+1);
                free(pLine->pLine);
                pLine->pLine = strdup(str);
            }
            
            // Next line
            pLine = pLine->pNext;
        }
    }
    else
    {
        // No adjustment needed.  Scan through the lines for any "%d(sp)" and
        // remove the '$'
        pLine = pFrame->pAsmLines->pNext;
        while (pLine != pFrame->pAsmLines)
        {
            if (pLine->pLine[14] == '$')
            {
                strncpy(str, pLine->pLine, 14);
                str[14] = 0;
                strcat(&str[14], &pLine->pLine[15]);
                free(pLine->pLine);
                pLine->pLine = strdup(str);
            }
      
            // Next line
            pLine = pLine->pNext;
        }
    }
}

/*
==========================================================================================
Remove dead "ads  0"
==========================================================================================
*/
static int remove_ads0_lines(void)
{
    asm_line_t  *pLine;
    asm_line_t  *pPrev;
    int         changes = 0;

    pLine = pFrame->pAsmLines->pNext;
    while (pLine != pFrame->pAsmLines)
    {
        if (strstr(pLine->pLine, "ads       0") != NULL)
        {
            // Delete the pLine, keeping our iteration valid
            pPrev = pLine->pPrev;
            delete_asm_line(pLine);
            pLine = pPrev;
            changes++;
        }
  
        // Next line
        pLine = pLine->pNext;
    }

    return changes;
}

/*
==========================================================================================
Generate the prolog code for a function.
==========================================================================================
*/
static void emit_func_prologue(Node *func)
{
    SAVE;
    if (strcmp(gpCurrSegment, ".text") != 0)
    {
      gpCurrSegment = ".text";
      emit("\n    .section .text");
    }
    if (!func->ty->isstatic)
        emit_noindent("\n    .public %s", func->fname);
    else
        emit_noindent("\n    .local %s", func->fname);
    maybe_print_source_loc(func);
    gMaybeEmitLoc = "";
    gMaybeEmitLine = "";
    emit_noindent("\n%s:", func->fname);
    localFuncs[nLocalFuncs++] = strdup(func->fname);

    calc_func_params(func->params);

#if 0
    if (func->ty->hasva) {
        printf("here\n");
        set_reg_nums(func->params);
        off -= emit_regsave_area();
    }
#endif

    int size;
    int off = 0;
    pFrame->nlvars = vec_len(func->localvars);
    for (int i = 0; i < pFrame->nlvars; i++)
    {
        Node *v = vec_get(func->localvars, i);
        size = v->ty->size;
        v->ty->isparam = false;
        pFrame->lvars[i].stackPos = off;
        pFrame->lvars[i].size = size;
        pFrame->lvars[i].kind = v->ty->kind;
        pFrame->lvars[i].name = v->varname;
        pFrame->lvars[i].assigned = 0;
        pFrame->lvars[i].used = 0;
        pFrame->lvars[i].useBeforeAssignWarned = 0;
        pFrame->lvars[i].notUsedWarn = 0;
        v->loff = off;
        v->isParam = 0;

        // Advance offset
        off += size;
        pFrame->localArea += size;
    }
    
    // Add localarea to all param offsets
    for (int i = 0; i < vec_len(func->params); i++)
    {
        Node *v = vec_get(func->params, i);
        v->loff += pFrame->localArea;
    }

    emit("ads       %d", -pFrame->localArea);
    pFrame->pAdsLine = pFrame->pAsmLines->pPrev;
    pFrame->stackOps = 0;
}

/*
==========================================================================================
Count jal, br, bnz, bnc references to labels
==========================================================================================
*/
void count_label_refs(void)
{
    asm_line_t  *pLine; 
    label_ref_t *pRef;
    char        *pS;

    // Scan all lines seaching for iftt, ldz, jal sequence
    pLine = pFrame->pAsmLines->pNext;
    while (pLine != pFrame->pAsmLines)
    {
        pS = &pLine->pLine[4];

        if (strncmp(pS, "jal", 3) == 0 ||
            strncmp(pS, "br ", 3) == 0 ||
            strncmp(pS, "bnc", 3) == 0 ||
            strncmp(pS, "bnz", 3) == 0)
        {
            pRef = NULL;

            // Test if branch / jump is non-numeric
            if (!isdigit(pS[10]))
            {
                // Search for label in our references
                pRef = pFrame->pLabelRefs;
                while (pRef != NULL)
                {
                    if (strcmp(pRef->label, &pS[10]) == 0)
                        break;
                    pRef = pRef->pNext;
                }
            }

            // Test if label found
            if (pRef == NULL)
            {
                pRef = (label_ref_t *) malloc(sizeof(label_ref_t));
                pRef->pNext = pFrame->pLabelRefs;
                pRef->refCount = 0;
                strcpy(pRef->label, &pS[10]);
                pFrame->pLabelRefs = pRef;
            }

            if (pRef)
                pRef->refCount++;
        }

        // Get the line
        pLine = pLine->pNext;
    }
}

/*
==========================================================================================
Find the specified label reference
==========================================================================================
*/
label_ref_t * find_label_ref(char *pStr)
{
    label_ref_t *pRef;

    // Search through all label refs
    pRef = pFrame->pLabelRefs;
    while (pRef != NULL)
    {
        if (strcmp(pRef->label, pStr) == 0)
            return pRef;

        pRef = pRef->pNext;
    }

    return NULL;
}

/*
==========================================================================================
Optimize unused labels by removing them
==========================================================================================
*/
int optimize_unused_labels(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    char        *pStr;
    char        jumpLabel[256];
    int         changes = 0;
    int         x;

    // Scan all lines seaching for labels
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Skip leading \n characters
        for (x = 0; pL1->pLine[x] == '\n'; x++)
            ;

        // Test if this line is a label
        if (strncmp(&pL1->pLine[x], "_L", 2) == 0 && strchr(&pL1->pLine[x], ':') != NULL)
        {
            // Copy label and remove the ':'
            strncpy(jumpLabel, &pL1->pLine[x], sizeof(jumpLabel));
            jumpLabel[strlen(jumpLabel)-1] = 0;

            // Scan through all lines and search for a jal / br to this label
            pL2 = pFrame->pAsmLines->pNext;
            while (pL2 != pFrame->pAsmLines)
            {
                if (pL2 != pL1 && (pStr = strstr(pL2->pLine, jumpLabel)) != NULL)
                {
                    if (strcmp(pStr, jumpLabel) == 0)
                        break;
                }

                // Next asm line
                pL2 = pL2->pNext;
            }

            // Test if we can remove this line
            if (pL2 == pFrame->pAsmLines)
            {
                // Delete this unused label line
                pL2 = pL1->pPrev;
                delete_asm_line(pL1);
                pL1 = pL2;
                changes++;
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Optimize multiple labels at the same point
==========================================================================================
*/
int optimize_multi_labels(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    asm_line_t  *pL3; 
    char        *pStr;
    char        firstLabel[256];
    char        secondLabel[256];
    char        newLine[256];
    int         changes = 0;
    int         x;
    int         y;

    // Scan all lines seaching for labels
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Skip leading \n characters
        for (x = 0; pL1->pLine[x] == '\n'; x++)
            ;

        // Test if this line is a label
        if (strncmp(&pL1->pLine[x], "_L", 2) == 0 && strchr(&pL1->pLine[x], ':') != NULL)
        {
            // Get the next line to see if it is also a label
            pL2 = pL1->pNext;

            // Skip leading \n characters
            for (y = 0; pL2->pLine[y] == '\n'; y++)
                ;

            // Test if next line is also a local label
            if (strncmp(&pL2->pLine[y], "_L", 2) == 0 && strchr(&pL2->pLine[y], ':') != NULL)
            {
                // Copy the first label and remove the ':'
                strncpy(firstLabel, &pL1->pLine[x], sizeof(firstLabel));
                firstLabel[strlen(firstLabel)-1] = 0;

                // Copy the second label and remove the ':'
                strncpy(secondLabel, &pL2->pLine[y], sizeof(secondLabel));
                secondLabel[strlen(secondLabel)-1] = 0;

                // Scan through all lines and search for a jal / br to the second label
                pL3 = pFrame->pAsmLines->pNext;
                while (pL3 != pFrame->pAsmLines)
                {
                    // Test for a jump to the second label
                    if (pL3 != pL2 && (pStr = strstr(pL3->pLine, secondLabel)) != NULL)
                    {
                        // Create new line with jump to firstLabel
                        *pStr = 0;
                        strncpy(newLine, pL3->pLine, sizeof(newLine));
                        free(pL3->pLine);
                        strcat(newLine, firstLabel);
                        pL3->pLine = strdup(newLine);
                        changes++;
                    }

                    // Next asm line
                    pL3 = pL3->pNext;
                }
                
                // Remove the pL2 label line
                delete_asm_line(pL2);
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Optimize jumps to jumps
==========================================================================================
*/
int optimize_label_jumps(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    label_ref_t *pLabelRef;
    char        newLine[256];
    char        *s1;
    char        *s2;
    int         changes = 0;

    // Scan all lines seaching for iftt, ldz, jal sequence
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        s1 = &pL1->pLine[4];
        if (strncmp(s1, "jal       _L", 12) == 0 || strncmp(s1, "br        _L", 12) == 0 ||
            strncmp(s1, "bnz       _L", 12) == 0 || strncmp(s1, "bz        _L", 12) == 0)
        {
            // Find the label reference
            pLabelRef = find_label_ref(&s1[10]);

            // Test if label is the very next line
            if (pLabelRef->pAsmLine == pL1->pNext)
            {
                pL2 = get_next_asm_line(pL1);
                delete_asm_line(pL1);
                pL1 = pL2;
                changes++;
                continue;
            }

            if (pLabelRef->pAsmLine)
            {
                // Get the line after the label
                pL2 = get_next_asm_line(pLabelRef->pAsmLine);

                // Test if the next line is also a JAL 
                s2 = &pL2->pLine[4];
                if (strncmp(s2, "jal       _L", 12) == 0 || strncmp(s2, "br        _L", 12) == 0)
                {
                    // Change the label reference of the first jal to the
                    // reference of the 2nd
                    pL1->pLine[14] = 0;
                    sprintf(newLine, "%s%s", pL1->pLine, &pL2->pLine[14]);
                    free(pL1->pLine);
                    pL1->pLine = strdup(newLine);
                    changes++;
                }
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Optimize orphaned jal opcodes caused by optimizing jumps
==========================================================================================
*/
int optimize_orphaned_jal(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    asm_line_t  *pL3; 
    char        *s2;
    int         changes = 0;

    // Scan all lines seaching for "jal       _L"
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Test if this line is a label
        if (strncmp(&pL1->pLine[4], "jal       _L", 12) == 0 ||
            strncmp(&pL1->pLine[4], "br        _L", 12) == 0)
        {
            // Get the previous line to test for br or "jal       _L"
            pL2 = get_prev_asm_line(pL1);
            if (pL2 == NULL)
            {
                pL1 = pL1->pNext;
                continue;
            }

            // Test if previous line is br or jal to local label
            s2 = &pL2->pLine[4];
            if (strncmp(s2, "br ", 3) == 0 || strncmp(s2, "jal       _L", 12) == 0)
            {
                // We can remove this jal only if it is not part of an if,
                // ifte or iftt
                pL3 = get_prev_asm_line(pL2);
                if (pL3 == NULL || strncmp(&pL3->pLine[4], "if", 2) == 0)
                {
                    pL1 = pL1->pNext;
                    continue;
                }

                // Test next previous opcode for ifte or iftt
                pL3 = get_prev_asm_line(pL3);
                if (pL3 == NULL || strncmp(&pL3->pLine[4], "ifte", 4) == 0 ||
                        strncmp(&pL3->pLine[4], "iftt", 4) == 0)
                {
                    pL1 = pL1->pNext;
                    continue;
                }

                // We can remove this jal
                delete_asm_line(pL1);
                pL1 = pL2;
                changes++;
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Optimize cpi 0 that follows and, or, xor
==========================================================================================
*/
int optimize_cpi_zero(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    char        *s2;
    int         changes = 0;

    // Scan all lines seaching for "cpi       0"
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Test if this line is a label
        if (strcmp(&pL1->pLine[4], "cpi       0") == 0)
        {
            // Get the previous line to test for and,or,xor
            pL2 = get_prev_asm_line(pL1);
            if (pL2 == NULL)
            {
                pL1 = pL1->pNext;
                continue;
            }

            // Test if previous line already updates flags
            s2 = &pL2->pLine[4];
            if (strncmp(s2, "and", 3) == 0 || strncmp(s2, "or ", 3) == 0 ||
                strncmp(s2, "xor", 3) == 0 || strncmp(s2, "ldax", 4) == 0)
            {
                // We can remove this cpi 0
                delete_asm_line(pL1);
                pL1 = pL2;
                changes++;
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Optimize remove extraneous loc.  The generator puts extraneous .loc statements in the
output.  All but the last are incorrect.
==========================================================================================
*/
int optimize_extraneous_loc(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    int         changes = 0;

    // Scan all lines seaching for "    .loc"
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Test if this line is a .loc line
        if (strncmp(pL1->pLine, "    .loc", 8) == 0)
        {
            // Get the line that is 2 lines after this
            pL2 = pL1->pNext->pNext;

            // Test if it is also a .loc line
            if (strncmp(pL2->pLine, "    .loc", 8) == 0)
            {
                // Test if one line has a declaration and the other has an 'if'
                if (strstr(pL1->pNext->pLine, "if") != NULL ||
                    strstr(pL1->pNext->pLine, "=") != NULL ||
                    strstr(pL1->pNext->pLine, "(") != NULL)
                {
                    // Test if the other line has int, char, etc.
                    if (strstr(pL2->pNext->pLine, "int") != NULL ||
                        strstr(pL2->pNext->pLine, "char") != NULL ||
                        strstr(pL2->pNext->pLine, "float") != NULL)
                    {
                        // We want to keep the first line
                        delete_asm_line(pL2->pNext);
                        delete_asm_line(pL2);
                        changes++;
                        pL1 = pL1->pNext;
                        continue;
                    }
                } 

                // Remove the first .loc line plus it's comment
                pL2 = pL1->pPrev;
                delete_asm_line(pL2->pNext);
                delete_asm_line(pL2->pNext);
                pL1 = pL2;
                changes++;
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Convert jal to br when jump distance allows
==========================================================================================
*/
int optimize_jal_to_br(void)
{
    asm_line_t  *pL1; 
    int         distance;
    int         changes = 0;

    // Scan all lines seaching for "    .loc"
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Test if this line is a "jal       _L" line
        if (strncmp(&pL1->pLine[4], "jal       _L", 12) == 0)
        {
            // Get the jump distance
            distance = calc_jal_distance(pL1);

            // If the distance is less 128 and greater than -129, then we can
            // convert to a br
            if (distance < 256 && distance > -257)
            {
                pL1->pLine[4] = 'b';
                pL1->pLine[5] = 'r';
                pL1->pLine[6] = ' ';
                changes++;
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Convert conditional if nz, if eq, etc. followed by br to a bnz
==========================================================================================
*/
int optimize_if_br(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    char        *s1;
    char        *s2;
    int         changes = 0;

    // Scan all lines seaching for "if "
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Test if this line is a "if        nz" line
        s1 = &pL1->pLine[4];
        if (strcmp(s1, "if        nz") == 0 ||
            strcmp(s1, "if        ne") == 0)
        {
            // Get the next asm line
            pL2 = get_next_asm_line(pL1);
            s2 = &pL2->pLine[4];

            // Test for br to local label
            if (strncmp(s2, "br        _L", 12) == 0)
            {
                // Convert the br / jal to bnz
                pL2->pLine[4] = 'b';
                pL2->pLine[5] = 'n';
                pL2->pLine[6] = 'z';

                // Delete the 'if'
                delete_asm_line(pL1);
                pL1 = pL2;
                changes++;
            }
        }

        else if (strcmp(s1, "if        z") == 0 ||
                 strcmp(s1, "if        eq") == 0)
        {
            // Get the next asm line
            pL2 = get_next_asm_line(pL1);
            s2 = &pL2->pLine[4];

            // Test for br to local label
            if (strncmp(s2, "br        _L", 12) == 0)
            {
                // Convert the br / jal to bnz
                pL2->pLine[4] = 'b';
                pL2->pLine[5] = 'z';
                pL2->pLine[6] = ' ';

                // Delete the 'if'
                delete_asm_line(pL1);
                pL1 = pL2;
                changes++;
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Convert notz conditional branch to just conditional ranch of opposite type
==========================================================================================
*/
int optimize_notz_br(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    char        *s1;
    char        *s2;
    int         changes = 0;

    // Scan all lines seaching for "ldz       ~z"
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Test if this line is a "ldz       ~z" line
        s1 = &pL1->pLine[4];
        if (strcmp(s1, "ldz       ~z") == 0)
        {
            // Get the next asm line
            pL2 = get_next_asm_line(pL1);
            s2 = &pL2->pLine[4];

            // Test for bz or bnz to local label
            if (strncmp(s2, "bz        _L", 12) == 0 ||
                strncmp(s2, "bnz       _L", 12) == 0)
            {
                // Convert the bz to bnz or bnz to bz
                if (pL2->pLine[5] == 'z')
                {
                    pL2->pLine[5] = 'n';
                    pL2->pLine[6] = 'z';
                }
                else
                {
                    pL2->pLine[5] = 'z';
                    pL2->pLine[6] = ' ';
                }

                // Delete the 'ldz       ~z'
                delete_asm_line(pL1);
                pL1 = pL2;
                changes++;
            }
        }

        else if (strcmp(s1, "if        z") == 0 ||
                 strcmp(s1, "if        eq") == 0)
        {
            // Get the next asm line
            pL2 = get_next_asm_line(pL1);
            s2 = &pL2->pLine[4];

            // Test for br to local label
            if (strncmp(s2, "br        _L", 12) == 0)
            {
                // Convert the br / jal to bnz
                pL2->pLine[4] = 'b';
                pL2->pLine[5] = 'z';
                pL2->pLine[6] = ' ';

                // Delete the 'if'
                delete_asm_line(pL1);
                pL1 = pL2;
                changes++;
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Convert bz around br to bnz, etc.
==========================================================================================
*/
int optimize_bz_br(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    label_ref_t *pLabelRef;
    char        *s1;
    char        *s2;
    int         changes = 0;

    // Scan all lines seaching for "bz" or "bnz"
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Test if this line is a bz / bnz
        s1 = &pL1->pLine[4];
        if (strncmp(s1, "bz ", 3) == 0 || strncmp(s1, "bnz", 3) == 0)
        {
            // Get the next asm line
            pL2 = get_next_asm_line(pL1);
            s2 = &pL2->pLine[4];

            // Test for bz or bnz to local label
            if (strncmp(s2, "br ", 3) == 0)
            {
                // Get pointer to the label
                pLabelRef = find_label_ref(&s1[10]);

                // Test if line after br is this label
                if (pL2->pNext == pLabelRef->pAsmLine)
                {
                    // Convert the br to bnz/bz (opposite of what was there
                    if (s1[1] == 'z')
                    {
                        s2[1] = 'n';
                        s2[2] = 'z';
                    }
                    else
                        s2[1] = 'z';

                    // Delete the "bz" / "bnz"
                    delete_asm_line(pL1);
                    pL1 = pL2;
                    changes++;
                }
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Remove sra / lra if funciton no longer uses jal or swap ra after other optimizaitons.
==========================================================================================
*/
int optimize_sra_lra(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    asm_line_t  *pSra; 
    asm_line_t  *pLine;
    int         raChangeCount;
    int         stackOffset;
    char        *s1;
    int         changes = 0;

    // Scan all lines seaching for "sra"
    pL1 = pFrame->pAsmLines->pNext;
    pSra = NULL;
    raChangeCount = 0;

    while (pL1 != pFrame->pAsmLines)
    {
        // Test if this line is an "sra" line
        s1 = &pL1->pLine[4];
        if (strcmp(s1, "sra") == 0)
            pSra = pL1;

        else if (strncmp(s1, "jal", 3) == 0)
            raChangeCount++;

        else if (strcmp(s1, "swap      ra") == 0)
            raChangeCount++;

        else if (strncmp(s1, "mul", 3) == 0)
            raChangeCount++;

        // When we find the lra, decide if we can remove both it and the 
        // associated sra depending if any raChange opcodes found
        else if (strcmp(s1, "lra") == 0 && pSra != NULL)
        {
            // If no raChange opcodes encountered, do processing
            if (raChangeCount == 0)
            {
                // Delete the sra line
                delete_asm_line(pSra);

                // Test for code to save 16-bit return value on stack
                if (strncmp(&pL1->pPrev->pLine[4], "ads", 3) == 0 &&
                    strncmp(&pL1->pPrev->pPrev->pLine[4], "swap", 4) == 0 &&
                    strncmp(&pL1->pPrev->pPrev->pPrev->pLine[4], "stax", 4) == 0)
                {
                    // We need to subtract 2 from the storage address since we
                    // are removing 2 bytes of stack space usage by removing
                    // the sra / lra opcodes
                    pL2 = pL1->pPrev->pPrev->pPrev;
                    stackOffset = atoi(&pL2->pLine[14]);
                    sprintf(&pL2->pLine[14], "%d(sp)", stackOffset - 2);
                }

                // Skip to the 'ret' opcode because we will delete this lra line
                pL1 = pL1->pNext;

                // Delete the 'lra' line
                delete_asm_line(pL1->pPrev);

                // Now we must adjust any stack relative variables because
                // we just changed the stack by 1.
                pLine = pFrame->pAsmLines->pNext;
                while (pLine != pFrame->pAsmLines)
                {
                    if (pLine->pLine[14] == '$')
                    {
                        int     off = atoi(&pLine->pLine[15]);
                        char    str[128];
                      
                        strncpy(str, pLine->pLine, 14);
                        str[14] = 0;
                        sprintf(&str[14], "$%d(sp)", off-1);
                        free(pLine->pLine);
                        pLine->pLine = strdup(str);
                    }
                    
                    // Next line
                    pLine = pLine->pNext;
                }

                changes++;
            }

            pSra = NULL;
            raChangeCount = 0;
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Optimize iftt, ldz, jal code generated by LOGAND and LOGOR 
==========================================================================================
*/
int optimize_logand_logor_iftt(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    asm_line_t  *pL3; 
    asm_line_t  *pLt; 
    asm_line_t  *pLt2; 
    label_ref_t *pLabelRef;
    char        *s1;
    char        *s2;
    char        *s3;
    int         loadZero;
    char        jumpLabel[256];
    int         resolved;
    int         substFound;
    int         changes = 0;

    // Scan all lines seaching for iftt, ldz, jal sequence
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Get the next 2 lines
        pL2 = get_next_asm_line(pL1);
        pL3 = get_next_asm_line(pL2);

        if (pL2 == NULL || pL3 == NULL)
            break;

        // Test for our sequence
        s1 = &pL1->pLine[4];
        s2 = &pL2->pLine[4];
        s3 = &pL3->pLine[4];
        if (strncmp(s1, "iftt", 4) == 0 && strncmp(s2, "ldz", 3) == 0 &&
            (strncmp(s3, "jal", 3) == 0 || strncmp(s3, "br ", 3) == 0))
        {
            // Found one!
            strcpy(jumpLabel, &s3[10]);
            strcat(jumpLabel, ":");
            if (strcmp(&s2[10], "1") == 0 || strcmp(&s2[10], "z") == 0)
                loadZero = 1;
            else
                loadZero = 0;

            resolved = 0;
            substFound = 0;
            pLt = pL3;
            while (!resolved)
            {
                // Scan forward looking for the jumpLabel
                while (pLt != NULL && strcmp(pLt->pLine, jumpLabel) != 0)
                    pLt = get_next_asm_line(pLt);
                
                // Validate label found
                if (!pLt)
                    break;

                // Get next line after the label line
                pLt2 = get_next_asm_line(pLt);        
                if (!pLt2)
                    break;
                // Skip any additional labels
                while (pLt2->pLine[0] != ' ')
                    pLt2 = get_next_asm_line(pLt2);

                // Test for "if        z" or "if        nz" line
                if ((strcmp(&pLt2->pLine[4], "if        z") == 0 ||
                    strcmp(&pLt2->pLine[4], "if        nz") == 0) &&
                    strncmp(&pLt2->pNext->pLine[4], "jal", 3) == 0)
                {
                    // Found a substitution!
                    substFound = 1;

                    // Test if this is a repeated hop to the next label
                    if ((pLt2->pLine[14] == 'z' && loadZero) || 
                        (pLt2->pLine[14] != 'z' && !loadZero))
                    {
                        // Setup to jump to the next label
                        strcpy(jumpLabel, &pLt2->pNext->pLine[14]);
                        strcat(jumpLabel, ":");
                        continue;
                    }
                    else
                    {
                        // We need to move or add a label after the pLt2->pNext jal,
                        // change the original pL3 to jump there, and delete the
                        // ldz line.  First remove the trailing ':'
                        jumpLabel[strlen(jumpLabel)-1] = 0;

                        // Get the label reference
                        pLabelRef = find_label_ref(jumpLabel);
                        if (pLabelRef->refCount == 1)
                        {
                            asm_line_t *pLbl = pLabelRef->pAsmLine;

                            // We can simply move the label.  Remove from list
                            pLbl->pPrev->pNext = pLbl->pNext;
                            pLbl->pNext->pPrev = pLbl->pPrev;

                            // And re-insert 
                            insert_asm_line_before(pLbl, pLt2->pNext->pNext);
                        }
                        else
                        {
                            // Replace the pL3 jump label
                            free(pL3->pLine);
                            strcat(jumpLabel, "b");
                            char line[512];
        emit("//here");
                            sprintf(line, "    jal       %s", jumpLabel);
                            pL3->pLine = strdup(line);

                            // Insert a new label
                            asm_line_t *pLbl = (asm_line_t *) malloc(sizeof(asm_line_t));
                            sprintf(line, "%s:", jumpLabel);
                            pLbl->pLine = strdup(line);
                            insert_asm_line_before(pLbl, pLt2->pNext->pNext);
                        }

                        // Delete the ldz line
                        delete_asm_line(pL2);

                        // Change the iftt to just an if
                        s1[2] = ' ';
                        s1[3] = ' ';
                        
                        changes++;
                        resolved = 1;
                    }
                }
                else if (substFound)
                {
                    // Remove reference to the label
                    pLabelRef = find_label_ref(&s3[10]);
                    if (pLabelRef && pLabelRef->refCount)
                    {
                        pLabelRef->refCount--;
                        if (pLabelRef->refCount == 0)
                            delete_asm_line(pLabelRef->pAsmLine);
                    }

                    // Change the L3 line to jump to the new label.  First
                    // remove the ':'
                    jumpLabel[strlen(jumpLabel)-1] = 0;

                    // Delete the existing line
                    free(pL3->pLine);
                    char line[512];
                    sprintf(line, "    jal       %s", jumpLabel);
                    pL3->pLine = strdup(line);

                    // Increment the ref count of the label we are jumping to
                    pLabelRef = find_label_ref(jumpLabel);
                    if (pLabelRef)
                        pLabelRef->refCount++;

                    // Delete the "ldz" line
                    delete_asm_line(pL2);

                    // Change the iftt to just an if
                    s1[2] = ' ';
                    s1[3] = ' ';
                    changes++;

                    // Advance the pL1 pointer
                    pL1 = pL3;

                    // Perform the substitution to the final label
                    resolved = 1;
                }
                else
                    // No changes
                    break;
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Optimize shr, shr, andi 1, if z
==========================================================================================
*/
int optimize_struct_masking(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    asm_line_t  *pL3; 
    char        *s1;
    char        *s2;
    char        *s3;
    int         shrCount = 0;
    int         changes = 0;

    // Scan all lines seaching for shr, andi, if
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Get the next 2 lines
        pL2 = get_next_asm_line_no_label(pL1);
        pL3 = get_next_asm_line_no_label(pL2);

        if (pL2 == NULL || pL3 == NULL)
            break;

        // Test for our sequence
        s1 = &pL1->pLine[4];
        s2 = &pL2->pLine[4];
        s3 = &pL3->pLine[4];
        if (strcmp(s1, "shr") == 0 && strncmp(s2, "andi", 4) == 0 &&
            (strcmp(s3, "if        z") == 0 || strcmp(s3, "if        nz") == 0))
        {
            int     andVal; 

            // We can remove the shr opcodes and just shift the andi value
            // left for the comparison
            while (strcmp(&pL2->pPrev->pLine[4], "shr") == 0)
                delete_asm_line(pL2->pPrev);

            andVal = atoi(&s2[10]);

            free(pL2->pLine);
            char line[32];
            sprintf(line, "    andi      %d", andVal << (shrCount + 1));
            pL2->pLine = strdup(line);

            changes++;

            pL1 = pL3;
        }
        else
        {
            if (strcmp(s1, "shr") == 0)
                shrCount++;
            else
                shrCount = 0;
        }

        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Optimize shr, shr, andi 1, if z
==========================================================================================
*/
int optimize_for_iftt(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    asm_line_t  *pL3; 
    char        *s1;
    char        *s2;
    char        jumpLabel[64];
    int         changes = 0;

    // Scan all lines seaching for shr, andi, if
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Get the next 2 lines
        pL2 = get_next_asm_line_no_label(pL1);

        if (pL2 == NULL)
            break;

        // Test for our sequence
        s1 = &pL1->pLine[4];
        s2 = &pL2->pLine[4];
        if (strncmp(s1, "if  ", 4) == 0 &&
            (strncmp(s2, "jal", 3) == 0 || strncmp(s2, "br ", 3) == 0))
        {
            // Get the jump label
            strcpy(jumpLabel, &s2[10]);
            strcat(jumpLabel, ":");

            // Get next line after the jump
            pL3 = get_next_asm_line(pL2);
            if (!pL3)
                break;
            if (strcmp(pL3->pLine, jumpLabel) == 0)
            {
                // Jump to next line???  That means neither the if nor the jal are needed

                pL1 = pL2;
                continue;
            }
            
            // Get the line two lines after the jal and test if it is the label
            pL3 = get_next_asm_line(pL3);
            if (!pL3)
                break;
            if (strcmp(pL3->pLine, jumpLabel) == 0)
            {
                // So we found this:
                //
                // if       cond
                // jal      _LBL
                // someop
                // _LBL:  
                //
                // We can reverse the if condition, delete the jal and let
                // someop be the target of the reversed if condition.
                reverse_if_comparison(pL1);

                // Reduce the jump label reference
                label_ref_t *pRef = find_label_ref(&s2[10]);
                if (pRef && pRef->refCount)
                {
                    pRef->refCount--;
                    if (pRef->refCount == 0)
                    {
                        // Delete the label line
                        delete_asm_line(pL3);
                    }
                }

                // Delete the jal line
                delete_asm_line(pL2);
                changes++;

                pL1 = pL1->pNext;
                continue;
            }

            // Test for iftt conversion
            // Get the line two lines after the jal and test if it is the label
            pL3 = get_next_asm_line(pL3);
            if (!pL3)
                break;
            if (strcmp(pL3->pLine, jumpLabel) == 0)
            {
                // So we found this:
                //
                // if       cond
                // jal      _LBL
                // someop
                // otherop
                // _LBL:  
                //
                // We can reverse the if condition change to iftt, delete the jal and let
                // someop be the target of the reversed if condition.
                reverse_if_comparison(pL1);
                convert_if_to_iftt(pL1);

                // Reduce the jump label reference
                label_ref_t *pRef = find_label_ref(&s2[10]);
                if (pRef && pRef->refCount)
                {
                    pRef->refCount--;
                    if (pRef->refCount == 0)
                    {
                        // Delete the label line
                        delete_asm_line(pL3);
                    }
                }

                // Delete the jal line
                delete_asm_line(pL2);
                changes++;
            }
        }

        // Next asm line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Optimize initializations with literals by re-arranging them so values with the same
acc load value are together.
==========================================================================================
*/
int optimize_literal_init(void)
{
    asm_line_t  *pL1; 
    asm_line_t  *pL2; 
    asm_line_t  *pL3; 
    asm_line_t  *pL4; 
    asm_line_t  *pL3Prev; 
    asm_line_t  *pLastLdi; 
    char        *s1;
    char        *s2;
    char        *s3;
    int         changes = 0;
    int         duplicates;
    int         ldiCount;
    int         ldiVal;
    int         asmLines;
    int         ldiHist[256];

    // Scan all lines seaching for shr, andi, if
    pL1 = pFrame->pAsmLines->pNext;
    while (pL1 != pFrame->pAsmLines)
    {
        // Get the next line
        pL2 = get_next_asm_line(pL1);
        if (pL2 == NULL)
            break;

        // Search for ldi lines followed by stax
        s1 = &pL1->pLine[4];
        s2 = &pL2->pLine[4];
        ldiCount = 0;
        asmLines = 0;
        duplicates = 0;
        memset(ldiHist, 0, sizeof(ldiHist));
        if (strncmp(s1, "ldi ", 4) == 0 && strncmp(s2, "stax", 4) == 0)
        {
            // Found potential start of a block of initialization
            ldiCount = 1;
            asmLines = 2;
            pLastLdi = pL1;

            ldiVal = atoi(&s1[10]);
            if (ldiVal < 256 && ldiVal >= -128)
            {
                if (++ldiHist[labs(ldiVal)] > 1)
                    duplicates++;
            }

            pL3 = get_next_asm_line(pL2);
            if (!pL3)
                break;
            s3 = &pL3->pLine[4];
            pL3Prev = pL3;
            while (strncmp(s3, "ldi ", 4) == 0 || strncmp(s3, "stax", 4) == 0)
            {
                // Keep track of the number of ldi operations
                if (strncmp(s3, "ldi ", 4) == 0)
                {
                    ldiCount++;
                    ldiVal = atoi(&s3[10]);
                    pLastLdi = pL3;
                    if (ldiVal < 256 && ldiVal >= -128)
                    {
                        if (++ldiHist[labs(ldiVal)] > 1)
                            duplicates++;
                    }
                }

                asmLines++;
                pL3Prev = pL3;
                pL3 = get_next_asm_line(pL3);
                if (!pL3)
                    break;
                s3 = &pL3->pLine[4];
            }

            // Skip processing if no duplicates
            if (duplicates == 0)
            {
                pL1 = pL3->pPrev;
                continue;
            }

            // Restore pL3 to the line with the last stax/ldi
            pL3 = pL3Prev;

            // We must keep the final ldi intact because other code
            // past the stax operation may also use that value.  So first
            // find any other ldi/stax with the same value and move those 
            // stax after the final ldi.
            pL2 = pL1;

            // Get value of the LDI
            ldiVal = atoi(&pLastLdi->pLine[14]);
            while (pL2 != pLastLdi)
            {
                // Test if this ldi value is the same as the last
                if (atoi(&pL2->pLine[14]) == ldiVal)
                {
                    // We can remove this ldi and move all of the following
                    // stax to after the pLastLdi
                    while (strncmp(&get_next_asm_line(pL2)->pLine[4], "stax", 4) == 0)
                    {
                        move_asm_line_after(get_next_asm_line(pL2), pLastLdi);
                        changes++;
                    }

                    // Now delete the ldi line.  If we are deleting the first
                    // line of the block, then we must reassign pL1
                    if (pL2 == pL1)
                        pL1 = get_next_asm_line(pL2);
                    asm_line_t *pTemp = get_next_asm_line(pL2);
                    delete_asm_line(pL2);
                    pL2 = pTemp;
                }
                else
                {
                    // Value does not match.  Advance to next "ldi"
                    pL2 = get_next_asm_line(pL2);
                    while (strncmp(&pL2->pLine[4], "stax", 4) == 0)
                        pL2 = get_next_asm_line(pL2);
                }
            }

            // Now scan the block and move any other duplicate ldi/stax 
            // together.
            pL2 = pL1;
            while (pL2 != pLastLdi)
            {
                // Get the value of this LDI
                ldiVal = atoi(&pL2->pLine[14]);

                // Find the last stax after this ldi
                pL3 = pL2;
                while (strncmp(&get_next_asm_line(pL3)->pLine[4], "stax ", 4) == 0)
                    pL3 = get_next_asm_line(pL3);

                // Find any matching ldi to pL2's ldi
                pL4 = get_next_asm_line(pL3);
                while (pL4 != pLastLdi)
                {
                    // Test if this ldi matches the pL2 LDI value
                    if (atoi(&pL4->pLine[14]) == ldiVal)
                    {
                        // Test if the LDI has a .loc and comment line
                        if (strncmp(pL4->pPrev->pPrev->pLine, "    .loc", 8) == 0)
                        {
                            // Move the .loc line
                            move_asm_line_after(pL4->pPrev->pPrev, pL3);
                            pL3 = pL3->pNext;
                        }
                        if (strncmp(pL4->pPrev->pLine, "    // ", 7) == 0)
                        {
                            // Move the comment line
                            move_asm_line_after(pL4->pPrev, pL3);
                            pL3 = pL3->pNext;
                        }

                        // Move this LDI's stax line and delete the ldi
                        while (strncmp(&get_next_asm_line(pL4)->pLine[4], "stax", 4) == 0)
                        {
                            move_asm_line_after(get_next_asm_line(pL4), pL3);
                            pL3 = pL3->pNext;
                            changes++;
                        }

                        asm_line_t *pTemp = get_next_asm_line(pL4);
                        delete_asm_line(pL4);
                        pL4 = pTemp;
                    }
                    else
                    {
                        // Advance to next "ldi"
                        pL4 = get_next_asm_line(pL4);
                        while (strncmp(&pL4->pLine[4], "stax", 4) == 0)
                            pL4 = get_next_asm_line(pL4);
                    }
                }

                // Advance to next "ldi"
                pL2 = get_next_asm_line(pL2);
                while (strncmp(&pL2->pLine[4], "stax", 4) == 0)
                    pL2 = get_next_asm_line(pL2);
            }
            
            printf("Found block %d lines with %d ldi and %d duplicates\n", 
                    asmLines, ldiCount, duplicates);

            // Advance the search pointer
            pL1 = pL3;
        }

        // Next line
        pL1 = pL1->pNext;
    }

    return changes;
}

/*
==========================================================================================
Determine opts base on optimization level
==========================================================================================
*/
static void populate_opts(opts_t *pOpt)
{
    // Set everything to zero
    memset(pOpt, 0, sizeof(opts_t));

    // Test for zero optimization
    if (gOptimizationLevel == '0')
        return;

    pOpt->ads0 = 1;
    pOpt->logand_logor = 1;
    pOpt->struct_masking = 1;

    switch (gOptimizationLevel)
    {
        case '2':
            pOpt->iftt = 1;
            pOpt->literal_init = 1;
            pOpt->label_jumps = 1;
            break;

        case 's':
            pOpt->iftt = 1;
            pOpt->literal_init = 1;
            break;
    }
}

/*
==========================================================================================
Perform known ASM optimizations prior to writing to the file
==========================================================================================
*/
static void perform_asm_optimizations(void)
{
    int     changes;
    opts_t  opt;

    // Count references to all labels
    count_label_refs();

    // Determine which opts to run based on optimization level
    populate_opts(&opt);

    do
    {
        changes = 0;

        // Remove unuseful "ads 0" lines
        if (opt.ads0)
            changes += remove_ads0_lines();

        // Remove unused labels
        changes += optimize_unused_labels();
        
        // Remove unused labels
        changes += optimize_multi_labels();
        
        // Remove orphaned jal
        changes += optimize_orphaned_jal();
        
        // Remove orphaned jal
        changes += optimize_cpi_zero();
        
        // Remove extraneous loc
        //changes += optimize_extraneous_loc();
        
        // Optimize jal to br
        changes += optimize_jal_to_br();
        
        // Optimize if followed by jal to br to local label
        changes += optimize_if_br();
        
        // Optimize notz followed by conditional branch
        changes += optimize_notz_br();

        // Optimize bz/bnz followed by br
        changes += optimize_bz_br();
        
        // Optimize inclusion of sra / lra depending if any jal / swap ra
        // opcodes left after other optimizations
        changes += optimize_sra_lra();
        
        // Optimize jump to jump
        if (opt.label_jumps)
            changes += optimize_label_jumps();
        
        // Optimize iftt, ldz, jal code generated by LOGAND and LOGOR 
        if (opt.logand_logor)
            changes += optimize_logand_logor_iftt();
        
        // Optimize iftt, ldz, jal code generated by LOGAND and LOGOR 
        if (opt.struct_masking)
            changes += optimize_struct_masking();

        // Find if / jal where if target is only one or two opcodes
        if (opt.iftt)
            changes += optimize_for_iftt();

        // Perform literal initialization optimization (re-ordering literal
        // initialization based on same value in acc).
        if (opt.literal_init)
            changes += optimize_literal_init();

    } while (changes > 0);
}

/*
==========================================================================================
Generate code for a top level node.  This will be a global or a function.
==========================================================================================
*/
void emit_toplevel(Node *v) {
    stack_frame_t frame;
    asm_line_t    *pLine;

    gLastEmitWasRet         = 0;
    gLastEmitWasJal         = 0;
    frame.stackPos          = 0;
    frame.localArea         = 0;
    frame.raDestroyed       = 0;
    frame.ixDestroyed       = 0;
    frame.nlvars            = 0;
    frame.nparam            = 0;
    frame.retCount          = 0;
    frame.nlocalLabels      = 0;
    frame.preserveVars      = 0;
    frame.nexternLabels     = 0;
    frame.lastSwapOptional  = 0;
    frame.isTernary         = 0;
    frame.emitCompZero      = 0;
    frame.noBitShiftStruc   = 0;
    frame.pAsmLines         = NULL;
    frame.pDataLines        = NULL;
    frame.pLabelRefs        = NULL;
    frame.accVal            = -1000;
    frame.accOnStack        = 0;
    frame.fname             = v->fname;
    frame.func              = v;
    pFrame = &frame;
      
    if (v->kind == AST_FUNC) {
        emit_func_prologue(v);
        emit_expr(v->body);
        emit_ret(pFrame->localArea, v->ty->rettype, pFrame->raDestroyed);

        // Check if ra or ix changed and finalize SP variable offsets
        adjust_stack_for_ra_change();

        // TODO:  Evaluate function relative jump distances

        // Perform asm optimization
        perform_asm_optimizations();

    } else if (v->kind == AST_DECL) {
        emit_global_var(v);
    } else {
        error("internal error");
    }

    // Write all pFrame->pAsmLines to the file
    pLine = frame.pAsmLines;
    while (pLine)
    {
        // Test for '$' stack modifiers
        if (pLine->pLine[14] == '$')
            memmove(&pLine->pLine[14], &pLine->pLine[15], 
                    strlen(&pLine->pLine[15])+1);
        fprintf(outputfp, "%s\n", pLine->pLine);
        pLine = pLine->pNext;
        if (pLine == frame.pAsmLines)
            pLine = NULL;
    }

    // Write all pFrame->pAsmLines to the file
    pLine = frame.pDataLines;
    if (pLine)
        if (strcmp(gpCurrSegment, ".data") != 0)
        {
          gpCurrSegment = ".data";
          fprintf(outputfp, "\n    .section .data\n\n");
        }
    while (pLine)
    {
        fprintf(outputfp, "%s\n", pLine->pLine);
        pLine = pLine->pNext;
        if (pLine == frame.pDataLines)
            pLine = NULL;
    }
    if (frame.pDataLines)
        fprintf(outputfp, "\n");
}

// vim: sw=4 ts=4

