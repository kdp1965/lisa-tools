/*
 * monitor.c - a small interactive monitor for the LISA core on the TT07
 * chip, compiled with sdcc -mlisa --tt07-cache (the 32K data space behind
 * the data cache: data below 0x3800, the stack at 0x7fff).
 *
 * Talks on UART1 (the debug UART the demo board forwards).  The first
 * Enter prints the bring-up banner, then "lisa> " waits for commands:
 *
 *   help                this list
 *   mr addr [len]       dump len (16) bytes of data RAM
 *   mw addr v [v ...]   write bytes from addr on
 *   stack [depth]       SP and the depth (32) bytes above it
 *   free                data end, the 0x3800 limit, the stack's use
 *   calc expr           an expression of real numbers, e.g. (2.3 + 5.6) * 2.11,
 *                       worked in bf16 on the FPU and in 32-bit software float,
 *                       each timed with TIMER1 (microseconds at 50 MHz)
 *   banner              the owl again
 *
 * Numbers are hex, with or without 0x (calc's are decimal).  Backspace
 * edits the line.
 */
#include <tt07.h>
#include <lisa/bf16.h>

/* ---- the UART ------------------------------------------------------- */

/* Output goes through a queue that is pumped while waiting for input, so
   that echoing a character never blocks the receive loop: a burst of
   input (a pasted line) arrives every 87 us at 115200 baud, which is how
   long one echoed character takes, and the receiver holds one byte. */
/* Everything the receive loop touches lives in one 64-byte block: the
   data cache has four 16-byte lines for the lower half of memory, indexed
   by address bits 5:4, so a block this size stays resident and a
   character never costs a line fill (50 us on the SPI RAM, against the
   87 us between characters). */
#define OQ 16
static struct {
  char line[40];
  char oq[OQ];
  unsigned char oq_head, oq_tail;
  unsigned char lo_byte, hi_byte;
} g;
#define line g.line
#define oq g.oq
#define oq_head g.oq_head
#define oq_tail g.oq_tail
#define lo_byte g.lo_byte
#define hi_byte g.hi_byte

static void oq_pump(void)
{
  if (oq_head != oq_tail && (UART1_STATUS & UART_TX_EMPTY)) {
    UART1_DATA = oq[oq_tail];
    oq_tail = (oq_tail + 1) & (OQ - 1);
  }
}

static void putc(char c)
{
  unsigned char next = (oq_head + 1) & (OQ - 1);
  while (next == oq_tail)
    oq_pump();
  oq[oq_head] = c;
  oq_head = next;
}

static void puts(const char *s)
{
  while (*s)
    putc(*s++);
}

static void putnl(void)
{
  putc('\r');
  putc('\n');
}

static char getc(void)
{
  while (!(UART1_STATUS & UART_RX_AVAIL))
    oq_pump();
  return UART1_DATA;
}

static void puthex2(unsigned char v)
{
  static const char hex[] = "0123456789abcdef";
  putc(hex[v >> 4]);
  putc(hex[v & 15]);
}

static void puthex4(unsigned int v)
{
  puthex2(v >> 8);
  puthex2((unsigned char)v);
}

static void putdec(unsigned int v)
{
  char buf[6];
  unsigned char n = 0;
  do {
    buf[n++] = '0' + v % 10;
    v /= 10;
  } while (v);
  while (n)
    putc(buf[--n]);
}

/* ---- the banner (lisa_bringup's) ------------------------------------ */

static const char *const owl[] = {
  "     .{{{}}}}}}.",
  "    {{{{{}}}}}}}.",
  "   {{{{  {{{{{}}}}",
  "  }}}}} _   _ {{{{{",
  "  }}}}  6   6  }}}}",
  " {{{{C    ^    {{{{",
  "}}}}}}\\  '='  /}}}}}",
  "{{{{{{{;.___.;}}}}}}",
  " {{{{{{{)   (}}}}}}'",
  "  ''\"''':   :''''''",
  "  jgs    `@` ",
  0,
};

static void banner(void)
{
  const char *const *p;
  putnl();
  for (p = owl; *p; p++) {
    puts(*p);
    putnl();
  }
  putnl();
  puts("Hello from TT07 LISA!");
  putnl();
  puts("lisa_monitor, sdcc -mlisa --tt07-cache: type help");
  putnl();
}

/* ---- what the linker and the core know ------------------------------ */

#define DATA_LIMIT 0x3800u          /* --tt07-cache, 2K stack: the stack's alias */
#define STACK_TOP  0x7fffu

/* the end of the data areas: s_FINITIALIZED + l_FINITIALIZED (the last area) */
static unsigned int data_end(void)
{
  unsigned int s, l;
  __asm
    ldi  #<s_FINITIALIZED
    sta  _g + 58
    ldi  #>s_FINITIALIZED
    sta  _g + 59
  __endasm;
  s = ((unsigned int)hi_byte << 8) | lo_byte;
  __asm
    ldi  #<l_FINITIALIZED
    sta  _g + 58
    ldi  #>l_FINITIALIZED
    sta  _g + 59
  __endasm;
  l = ((unsigned int)hi_byte << 8) | lo_byte;
  return s + l;
}

static unsigned int get_sp(void)
{
  __asm
    spix
    txa
    sta  _g + 58
    txau
    andi #0x7f
    sta  _g + 59
  __endasm;
  return ((unsigned int)hi_byte << 8) | lo_byte;
}

/* ---- the line and its words ----------------------------------------- */

static void readline(void)
{
  unsigned char n = 0;
  char c;
  for (;;) {
    c = getc();
    if (c == '\r' || c == '\n') {
      line[n] = 0;
      putnl();
      return;
    }
    if (c == 8 || c == 127) {
      if (n) {
        n--;
        puts("\b \b");
      }
      continue;
    }
    if (c < ' ')
      continue;
    if (n < sizeof line - 1) {
      line[n++] = c;
      putc(c);
    }
  }
}

static char *skip(char *p)
{
  while (*p == ' ')
    p++;
  return p;
}

/* a hex number at *pp; returns 0 if there is none */
static unsigned char number(char **pp, unsigned int *v)
{
  char *p = skip(*pp);
  unsigned int r = 0;
  unsigned char any = 0;
  if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
    p += 2;
  for (;; p++) {
    char c = *p;
    unsigned char d;
    if (c >= '0' && c <= '9') d = c - '0';
    else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
    else break;
    r = (r << 4) | d;
    any = 1;
  }
  if (!any || (*p && *p != ' '))
    return 0;
  *pp = p;
  *v = r;
  return 1;
}

static unsigned char word_is(const char *p, const char *w)
{
  while (*w)
    if (*p++ != *w++)
      return 0;
  return *p == 0 || *p == ' ';
}

/* ---- the commands --------------------------------------------------- */

static void dump(unsigned int addr, unsigned int len)
{
  while (len) {
    unsigned char i, n = len > 16 ? 16 : len;
    puthex4(addr);
    puts(": ");
    for (i = 0; i < n; i++) {
      puthex2(*(volatile unsigned char *)(addr + i));
      putc(' ');
    }
    for (i = n; i < 16; i++)
      puts("   ");
    putc('|');
    for (i = 0; i < n; i++) {
      unsigned char c = *(volatile unsigned char *)(addr + i);
      putc(c >= ' ' && c < 127 ? c : '.');
    }
    putc('|');
    putnl();
    addr += n;
    len -= n;
  }
}

static void cmd_mr(char *p)
{
  unsigned int addr, len = 16;
  if (!number(&p, &addr)) {
    puts("mr addr [len]");
    putnl();
    return;
  }
  number(&p, &len);
  dump(addr, len);
}

static void cmd_mw(char *p)
{
  unsigned int addr, v;
  unsigned char n = 0;
  if (!number(&p, &addr)) {
    puts("mw addr value [value ...]");
    putnl();
    return;
  }
  while (number(&p, &v)) {
    *(volatile unsigned char *)(addr + n) = (unsigned char)v;
    n++;
  }
  if (!n) {
    puts("mw addr value [value ...]");
    putnl();
    return;
  }
  dump(addr, n);
}

static void cmd_stack(char *p)
{
  unsigned int depth = 32, sp = get_sp();
  number(&p, &depth);
  puts("SP ");
  puthex4(sp);
  puts(" (");
  putdec(STACK_TOP - sp);
  puts(" bytes in use); above it:");
  putnl();
  if (depth > STACK_TOP - sp)
    depth = STACK_TOP - sp;
  dump(sp + 1, depth);
}

static void cmd_free(void)
{
  unsigned int end = data_end(), sp = get_sp();
  puts("data  0000-");
  puthex4(end - 1);
  puts(" (");
  putdec(end);
  puts(" bytes of globals)");
  putnl();
  puts("free  ");
  puthex4(end);
  puts("-");
  puthex4(DATA_LIMIT - 1);
  puts(" (");
  putdec(DATA_LIMIT - end);
  puts(" bytes up to the stack's alias at ");
  puthex4(DATA_LIMIT);
  putc(')');
  putnl();
  puts("stack ");
  puthex4(STACK_TOP);
  puts(" down, SP ");
  puthex4(sp);
  puts(" (");
  putdec(STACK_TOP - sp);
  puts(" bytes in use of 2048)");
  putnl();
}

/* ---- calc: real arithmetic, bf16 on the FPU against float32 ---------- */

/* A recursive-descent parser over the line - expr: term {(+|-) term},
   term: factor {(*|/) factor}, factor: -factor | (expr) | number, with
   decimal numbers [digits][.digits] - compiles the expression to a little
   reverse-Polish program over a table of constants, each kept twice: as
   the C float (the 32-bit software library) and as a bf16_t, which
   <lisa/bf16.h> multiplies and divides on the chip's unit (the adder is
   software: the silicon's is defective); a number becomes bf16 by
   rounding its float.  The program is then run in each arithmetic under
   TIMER1, and once more with the pushes and pops alone, whose time is
   the interpreter's own and comes off both. */

#define CLOCK_HZ 50000000ul             /* the project clock the times assume */

enum { OP_NUM = 1, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_NEG };
#define NPROG  40
#define NCONST 20
#define NSTACK 8
static unsigned char prog[NPROG], nprog;
static float cf[NCONST];
static bf16_t cb[NCONST];
static unsigned char nconst;
static float fstk[NSTACK];
static bf16_t bstk[NSTACK];
static char *cp;                        /* the parser's cursor */
static unsigned char err;

static void emit(unsigned char op)
{
  if (nprog < NPROG)
    prog[nprog++] = op;
  else
    err = 1;
}

static void putdecl(unsigned long v)
{
  char buf[10];
  unsigned char n = 0;
  do {
    buf[n++] = '0' + (unsigned char)(v % 10);
    v /= 10;
  } while (v);
  while (n)
    putc(buf[--n]);
}

/* [-]int.frac with up to six fraction digits (trailing zeros dropped);
   from 1e9 up as d.dddddde+N; inf and nan by name */
static void put_float(float f)
{
  union { float f; unsigned long u; } u;
  unsigned long ip;
  unsigned char i, n, e = 0;
  char frac[6];
  u.f = f;
  if ((u.u & 0x7f800000ul) == 0x7f800000ul) {
    puts(u.u & 0x007ffffful ? "nan" : u.u & 0x80000000ul ? "-inf" : "inf");
    return;
  }
  if (!(u.u & 0x7ffffffful)) {            /* +0 and -0 alike (x - x comes back as -0) */
    puts("0.0");
    return;
  }
  if (u.u & 0x80000000ul) {
    putc('-');
    f = -f;
  }
  if (f >= 1000000000.0f)
    while (f >= 10.0f) {
      f /= 10.0f;
      e++;
    }
  ip = (unsigned long)f;
  f -= (float)ip;
  putdecl(ip);
  for (i = 0; i < 6; i++) {
    unsigned char d;
    f *= 10.0f;
    d = (unsigned char)f;
    frac[i] = (char)d;
    f -= (float)d;
  }
  for (n = 6; n > 1 && frac[n - 1] == 0; n--)
    ;
  putc('.');
  for (i = 0; i < n; i++)
    putc('0' + frac[i]);
  if (e) {
    puts("e+");
    putdec(e);
  }
}

static void put_hex32(float f)
{
  union { float f; unsigned long u; } u;
  u.f = f;
  puthex4((unsigned int)(u.u >> 16));
  puthex4((unsigned int)u.u);
}

static unsigned char digit(char c)
{
  return c >= '0' && c <= '9';
}

static void parse_number(void)
{
  unsigned long ip = 0, fp = 0;
  float p10 = 1.0f, f;
  unsigned char any = 0;
  while (digit(*cp)) {
    ip = ip * 10 + (*cp++ - '0');
    any = 1;
  }
  if (*cp == '.') {
    cp++;
    while (digit(*cp)) {
      if (p10 < 10000000.0f) {             /* seven digits is all a float holds */
        fp = fp * 10 + (*cp - '0');
        p10 *= 10.0f;
      }
      cp++;
      any = 1;
    }
  }
  if (!any || nconst == NCONST) {
    err = 1;
    return;
  }
  f = (float)ip + (float)fp / p10;
  cf[nconst] = f;
  cb[nconst] = bf16_from_float(f);
  emit(OP_NUM);
  emit(nconst++);
}

static void parse_expr(void);

static void parse_factor(void)
{
  cp = skip(cp);
  if (*cp == '-') {
    cp++;
    parse_factor();
    emit(OP_NEG);
    return;
  }
  if (*cp == '+') {
    cp++;
    parse_factor();
    return;
  }
  if (*cp == '(') {
    cp++;
    parse_expr();
    cp = skip(cp);
    if (*cp != ')') {
      err = 1;
      return;
    }
    cp++;
    return;
  }
  parse_number();
}

static void parse_term(void)
{
  parse_factor();
  for (;;) {
    char op;
    if (err)
      return;
    cp = skip(cp);
    op = *cp;
    if (op != '*' && op != '/')
      return;
    cp++;
    parse_factor();
    emit(op == '*' ? OP_MUL : OP_DIV);
  }
}

static void parse_expr(void)
{
  parse_term();
  for (;;) {
    char op;
    if (err)
      return;
    cp = skip(cp);
    op = *cp;
    if (op != '+' && op != '-')
      return;
    cp++;
    parse_term();
    emit(op == '+' ? OP_ADD : OP_SUB);
  }
}

/* The program: mode 0 in float32, 1 in bf16, 2 the pushes and pops alone
   (float-sized moves, no arithmetic) for the interpreter's own time.  The
   result is left at the bottom of the stack. */
static void run(unsigned char mode)
{
  unsigned char i, sp = 0;
  for (i = 0; i < nprog; i++) {
    unsigned char op = prog[i];
    if (op == OP_NUM) {
      unsigned char k = prog[++i];
      if (sp == NSTACK) {
        err = 1;
        return;
      }
      if (mode == 1)
        bstk[sp] = cb[k];
      else
        fstk[sp] = cf[k];
      sp++;
    } else if (op == OP_NEG) {
      if (mode == 0)
        fstk[sp - 1] = -fstk[sp - 1];
      else if (mode == 1)
        bstk[sp - 1] = bf16_neg(bstk[sp - 1]);
    } else {
      sp--;                             /* the right operand at sp, the left below it */
      if (mode == 0) {
        float a = fstk[sp - 1], b = fstk[sp];
        fstk[sp - 1] = op == OP_ADD ? a + b : op == OP_SUB ? a - b : op == OP_MUL ? a * b : a / b;
      } else if (mode == 1) {
        bf16_t a = bstk[sp - 1], b = bstk[sp];
        bstk[sp - 1] = op == OP_ADD ? bf16_add(a, b) : op == OP_SUB ? bf16_sub(a, b) :
                       op == OP_MUL ? bf16_mul(a, b) : bf16_div(a, b);
      } else
        fstk[sp - 1] = fstk[sp];
    }
  }
}

/* TIMER1 restarted with a tick every prediv + 1 clocks */
static void timer_set(unsigned int prediv)
{
  TIMER1_CTRL = 0;
  TIMER1_PREDIV_LO = (unsigned char)prediv;
  TIMER1_PREDIV_HI = prediv >> 8;
  TIMER1_DIV_LO = 1;
  TIMER1_DIV_HI = 0;
  TIMER1_COUNT = 0;
  TIMER1_CTRL = TIMER_CTRL_ENABLE;
}

/* the clocks run(mode) takes.  COUNT is 8 bits and a wrap cannot be
   seen, so a coarse pass with 1 ms ticks (up to 255 ms) finds the
   magnitude, and a second one uses the finest tick, 10 us at least, whose
   250 steps still cover it. */
static unsigned long timed(unsigned char mode)
{
  unsigned long tick;
  unsigned char n;
  timer_set(CLOCK_HZ / 1000 - 1);
  run(mode);
  n = TIMER1_COUNT;
  tick = ((unsigned long)n + 1) * (CLOCK_HZ / 1000) / 250;
  if (tick < CLOCK_HZ / 100000)
    tick = CLOCK_HZ / 100000;
  timer_set((unsigned int)(tick - 1));
  run(mode);
  n = TIMER1_COUNT;
  TIMER1_CTRL = 0;
  return (unsigned long)n * tick;
}

/* clocks as microseconds, or milliseconds with two decimals from 10 ms */
static void put_time(unsigned long clocks)
{
  unsigned long us = clocks / (CLOCK_HZ / 1000000);
  if (us < 10000) {
    putdecl(us);
    puts(" us");
    return;
  }
  putdecl(us / 1000);
  putc('.');
  us = us % 1000 / 10;
  putc('0' + us / 10);
  putc('0' + us % 10);
  puts(" ms");
}

static void cmd_calc(char *p)
{
  unsigned long ovh, tf, tb;
  float f, bf;
  cp = skip(p);
  err = 0;
  nprog = nconst = 0;
  if (!*cp) {
    puts("calc expr      + - * / ( ) and decimal numbers, e.g. calc (2.3 + 5.6) * 2.11");
    putnl();
    return;
  }
  parse_expr();
  cp = skip(cp);
  if (err || *cp) {
    puts("? at '");
    puts(cp);
    putc('\'');
    putnl();
    return;
  }
  ovh = timed(2);
  tf = timed(0);
  f = fstk[0];
  tb = timed(1);
  bf = bf16_to_float(bstk[0]);
  if (err) {
    puts("? too deep");
    putnl();
    return;
  }
  tf = tf > ovh ? tf - ovh : 0;
  tb = tb > ovh ? tb - ovh : 0;
  puts("float32 ");
  put_float(f);
  puts("  (");
  put_hex32(f);
  puts(")  in ");
  put_time(tf);
  putnl();
  puts("bf16    ");
  put_float(bf);
  puts("  (");
  puthex4(bstk[0]);
  puts(")  in ");
  put_time(tb);
  puts(", off by ");
  put_float(bf - f);
  putnl();
}

static void cmd_help(void)
{
  puts("help               this list");            putnl();
  puts("mr addr [len]      dump data RAM (hex, 16 bytes)"); putnl();
  puts("mw addr v [v ...]  write bytes");            putnl();
  puts("stack [depth]      SP and the bytes above it"); putnl();
  puts("free               data end, limit 3800, stack use"); putnl();
  puts("calc expr          real arithmetic: bf16 on the FPU vs float32, timed"); putnl();
  puts("banner             the owl");               putnl();
}

void main(void)
{
  unsigned char shown = 0;
  char *p;

  for (;;) {
    if (shown)
      puts("lisa> ");
    readline();
    if (!shown) {
      banner();
      shown = 1;
      continue;
    }
    p = skip(line);
    if (!*p)
      continue;
    if (word_is(p, "help") || word_is(p, "?"))
      cmd_help();
    else if (word_is(p, "mr"))
      cmd_mr(p + 2);
    else if (word_is(p, "mw"))
      cmd_mw(p + 2);
    else if (word_is(p, "stack"))
      cmd_stack(p + 5);
    else if (word_is(p, "free"))
      cmd_free();
    else if (word_is(p, "calc"))
      cmd_calc(p + 4);
    else if (word_is(p, "banner"))
      banner();
    else {
      puts("? ");
      puts(p);
      puts(" (help)");
      putnl();
    }
  }
}
