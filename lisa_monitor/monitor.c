/*
 * monitor.c - a small interactive monitor for the LISA core on the TT07
 * chip, in two builds:
 *
 *   monitor.ihx        sdcc -mlisa --tt07-cache: the 32K data space behind
 *                      the data cache (data below 0x3800, the stack at
 *                      0x7fff), which needs the RP2040's SPI RAM emulation
 *   monitor_small.ihx  sdcc -mlisa -DSMALL: the chip's 128-byte RAM, nothing
 *                      special on the board
 *
 * Talks on UART1 (the debug UART the demo board forwards).  The first
 * Enter prints the banner, then "lisa> " waits for commands:
 *
 *   help                this list
 *   mr addr [len]       dump len (16) bytes of data RAM
 *   mw addr v [v ...]   write bytes from addr on
 *   stack [depth]       SP and the depth (32) bytes above it
 *   free                data end, the limit, the stack's use and high water
 *   calc expr           an expression of real numbers, e.g. (2.3 + 5.6) * 2.11,
 *                       worked in bf16 on the FPU and (not in SMALL: the
 *                       software float's stack alone is the 128 bytes) in
 *                       32-bit software float, each timed with TIMER1
 *                       (microseconds at 50 MHz)
 *   banner              see LISA again
 *
 * Numbers are hex, with or without 0x (calc's are decimal).  Backspace
 * edits the line.
 */
#include <tt07.h>
#include <lisa/bf16.h>

/* ---- the UART ------------------------------------------------------- */

#ifdef SMALL
#define LINE 28                         /* "calc -2.5 * (4 - 6) + 0.1" is 25 */
#else
#define LINE 40
#endif

#ifndef SMALL
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
#endif
/* Defined at the end of the file, so that it is the last thing in the data
   and the line sits right below the stack: a command's line is dead once
   its words are parsed (calc compiles its expression first thing), and
   from then on the stack may grow into it - in the 128-byte build that is
   where the room for evaluating and printing comes from. */
struct gbuf {
  char line[LINE];
#ifndef SMALL
  char oq[OQ];
  unsigned char oq_head, oq_tail;
#endif
};
extern struct gbuf g;
#define line g.line
#define oq g.oq
#define oq_head g.oq_head
#define oq_tail g.oq_tail
#ifdef SMALL
/* the byte pair the inline asm fills: the end of the line, which no
   command reads once it has parsed its words (every data byte counts) */
#define lh ((unsigned char *)line + LINE - 2)
#else
static unsigned char lh[2];
#endif

#ifdef SMALL
static void putc(char c)
{
  while (!(UART1_STATUS & UART_TX_EMPTY))
    ;
  UART1_DATA = c;
}

static char getc(void)
{
  while (!(UART1_STATUS & UART_RX_AVAIL))
    ;
  return UART1_DATA;
}
#else
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

static char getc(void)
{
  while (!(UART1_STATUS & UART_RX_AVAIL))
    oq_pump();
  return UART1_DATA;
}
#endif

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

/* no buffer: the digits from the top, leading zeros held back (every byte
   of stack counts in the 128-byte build) */
static void putdec(unsigned int v)
{
  unsigned int p = 10000;
  unsigned char lead = 1;
  for (; p; p /= 10) {
    unsigned char d = v / p % 10;
    if (d || !lead || p == 1) {
      putc('0' + d);
      lead = 0;
    }
  }
}

static void pad(unsigned char n)
{
  while (n--)
    putc(' ');
}

static unsigned char digits(unsigned int v)
{
  unsigned char n = 1;
  while (v >= 10) {
    v /= 10;
    n++;
  }
  return n;
}

/* ---- the banner (lisa_bringup's) ------------------------------------ */

static const char *const lisa[] = {       /* LISA herself (jgs) */
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
  for (p = lisa; *p; p++) {
    puts(*p);
    putnl();
  }
  putnl();
  puts("Hello from TT07 LISA!");
  putnl();
#ifdef SMALL
  puts("lisa_monitor, sdcc -mlisa, the 128-byte RAM: type help");
#else
  puts("lisa_monitor, sdcc -mlisa --tt07-cache: type help");
#endif
  putnl();
}

/* ---- what the linker and the core know ------------------------------ */

#ifdef SMALL
#define DATA_LIMIT 0x80u            /* the 128-byte RAM: data up, the stack down */
#define STACK_TOP  0x7fu
#else
#define DATA_LIMIT 0x3800u          /* --tt07-cache, 2K stack: the stack's alias */
#define STACK_TOP  0x7fffu
#define STACK_LOW  0x7800u
#endif
#define FILL 0xa5                   /* painted below SP at the start: the high-water mark */

/* the end of the data in RAM: where SSEG, the one-byte area the linker
   places after DATA and INITIALIZED, starts */
#ifdef SMALL
#define STA_LH   sta  _g + (LINE - 2)
#define STA_LH1  sta  _g + (LINE - 1)
#else
#define STA_LH   sta  _lh
#define STA_LH1  sta  _lh + 1
#endif

static unsigned int data_end(void)
{
  __asm
    ldi  #<s_SSEG
    STA_LH
    ldi  #>s_SSEG
    STA_LH1
  __endasm;
  return ((unsigned int)lh[1] << 8) | lh[0];
}

static unsigned int get_sp(void)
{
  __asm
    spix
    txa
    STA_LH
    txau
    andi #0x7f
    STA_LH1
  __endasm;
  return ((unsigned int)lh[1] << 8) | lh[0];
}

#ifdef SMALL
#define stack_low() data_end()
#else
#define stack_low() STACK_LOW
#endif

/* the free bytes between the data and the stack take FILL; what the
   stack has reached since shows as the first byte that is not FILL */
static void stack_paint(void)
{
  unsigned int a, sp = get_sp();
  for (a = stack_low(); a <= sp; a++)
    *(volatile unsigned char *)a = FILL;
}

static unsigned int stack_high(void)
{
  unsigned int a;
  for (a = stack_low(); a < STACK_TOP && *(volatile unsigned char *)a == FILL; a++)
    ;
  return a;
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
  unsigned int end = data_end(), sp = get_sp(), high = stack_high();
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
#ifdef SMALL
  puts(" bytes, the stack's; so is the line once parsed)");
#else
  puts(" bytes up to the stack's alias at ");
  puthex4(DATA_LIMIT);
  putc(')');
#endif
  putnl();
  puts("stack ");
  puthex4(STACK_TOP);
  puts(" down, SP ");
  puthex4(sp);
  puts(" (");
  putdec(STACK_TOP - sp);
  puts(" bytes in use of ");
  putdec(STACK_TOP - stack_low() + 1);
  puts("), high water ");
  putdec(STACK_TOP - high + 1);
  puts(" at ");
  puthex4(high);
  putnl();
}

/* ---- calc: real arithmetic, bf16 on the FPU against float32 ---------- */

/* A precedence-climbing parser over the line - factor: -factor | (expr) |
   number, then operators by precedence (* / over + -), with decimal
   numbers [digits][.digits] - compiles the expression to a little
   reverse-Polish program over a table of constants, each kept as a bf16_t,
   which <lisa/bf16.h> multiplies and divides on the chip's unit (the
   adder is software: the silicon's is defective), and, except in SMALL,
   as the C float (the 32-bit software library) too, the bf16 then being
   the float rounded.  In SMALL the decimal digits are turned into bf16
   and back with the unit itself.  The program is then run in each
   arithmetic under TIMER1, and once more with the pushes and pops alone,
   whose time is the interpreter's own and comes off both. */

#define CLOCK_HZ 50000000ul             /* the project clock the times assume */

enum { OP_NUM = 1, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_NEG };
#ifdef SMALL
#define NPROG  14                       /* 72 bytes of data with the line; the rest of the 128 is the stack's */
#define NCONST 5
#define NSTACK 4
#else
#define NPROG  40
#define NCONST 20
#define NSTACK 8
#endif
static unsigned char prog[NPROG], nprog;
static bf16_t cb[NCONST];
static unsigned char nconst;
static bf16_t bstk[NSTACK];
#ifndef SMALL
static float cf[NCONST];
static float fstk[NSTACK];
#endif
static char *cp;                        /* the parser's cursor */
static unsigned char err;               /* 1: a syntax error at cp, 2: too deep */
#define NOPS 6
static unsigned char ops[NOPS], nops;   /* the operators waiting, parentheses among them */

static void emit(unsigned char op)
{
  if (nprog < NPROG)
    prog[nprog++] = op;
  else
    err = 2;
}

#ifndef SMALL
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

static unsigned char digitsl(unsigned long v)
{
  unsigned char n = 1;
  while (v >= 10) {
    v /= 10;
    n++;
  }
  return n;
}

/* [-]int.frac with up to six fraction digits (trailing zeros dropped);
   from 1e9 up as d.dddddde+N; inf and nan by name.  Padded to width
   characters (the columns after it line up). */
static void put_float(float f, unsigned char width)
{
  union { float f; unsigned long u; } u;
  unsigned long ip;
  unsigned char i, n, w, e = 0;
  char frac[6];
  u.f = f;
  if ((u.u & 0x7f800000ul) == 0x7f800000ul) {
    const char *s = u.u & 0x007ffffful ? "nan" : u.u & 0x80000000ul ? "-inf" : "inf";
    puts(s);
    w = s[0] == '-' ? 4 : 3;
    if (w < width)
      pad(width - w);
    return;
  }
  if (!(u.u & 0x7ffffffful)) {            /* +0 and -0 alike (x - x comes back as -0) */
    puts("0.0");
    if (width > 3)
      pad(width - 3);
    return;
  }
  w = 0;
  if (u.u & 0x80000000ul) {
    putc('-');
    w++;
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
  w += digitsl(ip);
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
  w += 1 + n;
  if (e) {
    puts("e+");
    putdec(e);
    w += 2 + digits(e);
  }
  if (w < width)
    pad(width - w);
}

static void put_hex32(float f)
{
  union { float f; unsigned long u; } u;
  u.f = f;
  puthex4((unsigned int)(u.u >> 16));
  puthex4((unsigned int)u.u);
}
#endif

/* a bf16 as [-]int.frac, up to six fraction digits (trailing zeros
   dropped), from its bits alone: the eight-bit mantissa with its hidden
   bit is an integer over 2^s, s set by the exponent, and a decimal digit
   is a multiply by ten and a shift - no arithmetic library, next to no
   stack (the 128-byte build).  65536 and up as "big" - the bits are
   printed beside it anyway.  Padded to width. */
static void put_bf16(bf16_t b, unsigned char width)
{
  unsigned char i, s, w = 0, zeros, any;
  unsigned int m;
  if (bf16_isnan(b)) {
    puts("nan");
    w = 3;
  } else if (bf16_isinf(b)) {
    puts(bf16_signbit(b) ? "-inf" : "inf");
    w = bf16_signbit(b) ? 4 : 3;
  } else if (bf16_iszero(b)) {
    puts("0.0");
    w = 3;
  } else {
    if (bf16_signbit(b)) {
      putc('-');
      w++;
      b = bf16_abs(b);
    }
    s = b >> 7;                         /* the biased exponent: 127 for 1.x */
    m = 0x80 | (b & 0x7f);              /* 1.xxxxxxx as an integer over 2^7 */
    if (s >= 127 + 16) {
      puts("big");
      w += 3;
    } else {
      if (s >= 134) {                   /* 128 and up: whole */
        m <<= s - 134;
        s = 0;
      } else {
        s = 134 - s;                    /* the fraction's bits */
        while (s > 12) {                /* keep m * 10 in 16 bits (the value is under 1/32 then) */
          m >>= 1;
          s--;
        }
      }
      putdec(m >> s);
      w += digits(m >> s) + 1;
      putc('.');
      m &= (1u << s) - 1;
      zeros = 0;                        /* trailing zeros held back: one stays if nothing else comes */
      any = 0;
      for (i = 0; i < 6; i++) {
        unsigned char d;
        m *= 10;
        d = (unsigned char)(m >> s);
        m &= (1u << s) - 1;
        if (!d)
          zeros++;
        else {
          for (; zeros; zeros--, w++)
            putc('0');
          putc('0' + d);
          w++;
          any = 1;
        }
      }
      if (!any) {
        putc('0');
        w++;
      }
    }
  }
  if (w < width)
    pad(width - w);
}

static unsigned char digit(char c)
{
  return c >= '0' && c <= '9';
}

#ifdef SMALL
/* scaled / p10 as a bf16 rounded to nearest even, by long division in
   16-bit integers: the eight leading bits of the quotient and a guard bit,
   the remainder as the sticky.  Exact, unlike the unit's divide, which
   slips an ulp now and then (1.5 came out 1.4921875). */
#define q   bstk[0]                        /* its operands and working values in the idle evaluation stack */
#define p10 bstk[1]
#define r   bstk[2]
#define mant cb[nconst]                    /* accumulated where the constant will go */
static bf16_t bf16_ratio(void)
{
  signed char e = -1;
  unsigned char bits = 0, b = 15;
  mant = 0;
  r = q % p10;
  q = q / p10;
  if (q) {                                 /* the integer bits, from the top one */
    while (!(q >> b))
      b--;
    e = b;
    do {
      if (bits < 9) {
        mant = (mant << 1) | ((q >> b) & 1);
        bits++;
      } else if ((q >> b) & 1)
        r = 1;                             /* a dropped bit: sticky (p10 >= 1 makes any r != 0 sticky) */
    } while (b--);
  }
  while (bits < 9) {                       /* the fraction bits: r * 2 / p10 */
    unsigned char bit;
    r <<= 1;
    bit = r >= p10;
    if (bit)
      r -= p10;
    if (bits || bit) {                     /* the first one found is the hidden bit */
      mant = (mant << 1) | bit;
      bits++;
    } else if (--e < -126)
      return 0;
  }
  if ((mant & 1) && (r || (mant & 2)))     /* the guard bit: round up on more than half, or half to even */
    mant += 2;
  mant >>= 1;
  if (mant & 0x100) {                      /* rounded up into the next power of two */
    mant >>= 1;
    e++;
  }
  return (bf16_t)(((unsigned int)(e + 127) << 7) | (mant & 0x7f));
}
#undef q
#undef p10
#undef r
#undef mant
#endif

static void parse_number(void)
{
  unsigned char any = 0;
#ifdef SMALL
  /* the digits as one integer scaled by a power of ten - fraction digits
     taken while that stays in 16 bits - then one divide on the unit; the
     two integers live in the evaluation stack, idle while parsing (every
     byte of frame counts here) */
#define scaled bstk[0]
#define p10    bstk[1]
  scaled = 0;
  p10 = 1;
  while (digit(*cp)) {
    if (scaled > 6553 || (scaled == 6553 && *cp > '5')) {   /* past 65535 */
      err = 1;
      return;
    }
    scaled = scaled * 10 + (*cp++ - '0');
    any = 1;
  }
  if (*cp == '.') {
    cp++;
    while (digit(*cp)) {
      if (scaled <= 6552) {
        scaled = scaled * 10 + (*cp - '0');
        p10 *= 10;
      }
      cp++;
      any = 1;
    }
  }
  if (!any || nconst == NCONST) {
    err = 1;
    return;
  }
  cb[nconst] = bf16_ratio();
#undef scaled
#undef p10
#else
  unsigned long ip = 0, fp = 0;
  float p10 = 1.0f, f;
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
#endif
  emit(OP_NUM);
  emit(nconst++);
}

/* Shunting-yard, no recursion: a number goes straight into the program,
   an operator waits in a small stack until one that binds no tighter
   follows it, a parenthesis is a marker there and the unary minus the
   tightest operator of all - so a parenthesised expression costs a byte
   of data, not a chain of frames (the 128-byte build's stack). */
#define OP_PAREN 0x80

static unsigned char prec(unsigned char op)
{
  return op == OP_NEG ? 3 : op >= OP_MUL ? 2 : 1;
}

static void push_op(unsigned char op)
{
  if (nops < NOPS)
    ops[nops++] = op;
  else
    err = 2;
}

static void parse(void)
{
  unsigned char code, want = 1;         /* a value is due: a number, a unary sign or a parenthesis */
  nops = 0;
  for (;;) {
    cp = skip(cp);
    if (want) {
      if (*cp == '-') {
        push_op(OP_NEG);
        cp++;
      } else if (*cp == '+')
        cp++;
      else if (*cp == '(') {
        push_op(OP_PAREN);
        cp++;
      } else {
        parse_number();
        want = 0;
      }
      if (err)
        return;
      continue;
    }
    if (*cp == ')') {
      while (nops && ops[nops - 1] != OP_PAREN)
        emit(ops[--nops]);
      if (!nops) {                      /* no parenthesis open */
        err = 1;
        return;
      }
      nops--;
      cp++;
      continue;
    }
    code = *cp == '+' ? OP_ADD : *cp == '-' ? OP_SUB : *cp == '*' ? OP_MUL : *cp == '/' ? OP_DIV : 0;
    if (!code)
      break;                            /* the end, or what cmd_calc reports */
    cp++;
    while (nops && ops[nops - 1] != OP_PAREN && prec(ops[nops - 1]) >= prec(code))
      emit(ops[--nops]);
    push_op(code);
    want = 1;
  }
  if (want) {                           /* ended where a value was due */
    err = 1;
    return;
  }
  while (nops) {
    if (ops[nops - 1] == OP_PAREN) {    /* one never closed */
      err = 1;
      return;
    }
    emit(ops[--nops]);
  }
}

/* The program: mode 0 in float32 (not in SMALL), 1 in bf16, 2 the pushes
   and pops alone (no arithmetic) for the interpreter's own time.  The
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
#ifndef SMALL
      if (mode != 1)
        fstk[sp] = cf[k];
      else
#endif
        bstk[sp] = cb[k];
      sp++;
    } else if (op == OP_NEG) {
#ifndef SMALL
      if (mode == 0)
        fstk[sp - 1] = -fstk[sp - 1];
      else
#endif
      if (mode == 1)
        bstk[sp - 1] = bf16_neg(bstk[sp - 1]);
    } else {
      sp--;                             /* the right operand at sp, the left below it */
#ifndef SMALL
      if (mode == 0) {
        float a = fstk[sp - 1], b = fstk[sp];
        fstk[sp - 1] = op == OP_ADD ? a + b : op == OP_SUB ? a - b : op == OP_MUL ? a * b : a / b;
      } else
#endif
      if (mode == 1) {
        switch (op) {                   /* a switch: the conditional expression's temporaries cost frame */
        case OP_ADD: bstk[sp - 1] = bf16_add(bstk[sp - 1], bstk[sp]); break;
        case OP_SUB: bstk[sp - 1] = bf16_sub(bstk[sp - 1], bstk[sp]); break;
        case OP_MUL: bstk[sp - 1] = bf16_mul(bstk[sp - 1], bstk[sp]); break;
        default:     bstk[sp - 1] = bf16_div(bstk[sp - 1], bstk[sp]); break;
        }
      } else {
#ifndef SMALL
        fstk[sp - 1] = fstk[sp];
#else
        bstk[sp - 1] = bstk[sp];
#endif
      }
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

/* the time run(mode) takes, in 10 us units - 16-bit arithmetic throughout,
   the 32-bit helpers' frames being what the 128-byte build cannot afford.
   COUNT is 8 bits and a wrap cannot be seen, so a coarse pass with 1 ms
   ticks (up to 255 ms) finds the magnitude, and a second one uses the
   finest tick, 10 us at least, whose 250 steps still cover it. */
#define TICK10 ((unsigned int)(CLOCK_HZ / 100000))      /* clocks in 10 us */
static unsigned int timed(unsigned char mode)
{
  unsigned int tick;                    /* in 10 us units, 1..103 */
  unsigned char n;
  timer_set((unsigned int)(CLOCK_HZ / 1000) - 1);
  run(mode);
  n = TIMER1_COUNT;
  tick = (((unsigned int)n + 1) * 100 + 249) / 250;   /* rounded up: a tick short and the count wraps */
  timer_set(tick * TICK10 - 1);
  run(mode);
  n = TIMER1_COUNT;
  TIMER1_CTRL = 0;
  return n * tick;
}

/* 10 us units as microseconds, or milliseconds with two decimals from
   10 ms, right-aligned in nine characters */
static void put_time(unsigned int t10)
{
  if (t10 < 1000) {
    pad(6 - digits(t10 * 10));
    putdec(t10 * 10);
    puts(" us");
    return;
  }
  pad(3 - digits(t10 / 100));
  putdec(t10 / 100);
  putc('.');
  t10 %= 100;
  putc('0' + t10 / 10);
  putc('0' + t10 % 10);
  puts(" ms");
}

static void cmd_calc(char *p)
{
#ifdef SMALL
#define ovh (*(unsigned int *)(line + LINE - 4))   /* the interpreter's time: in the line, dead once compiled */
  unsigned int tb;
#else
  unsigned int ovh, tb, tf;
  float f, bf;
#endif
  cp = skip(p);
  err = 0;
  nprog = nconst = 0;
  if (!*cp) {
    puts("calc expr      + - * / ( ) and decimal numbers, e.g. calc (2.3 + 5.6) * 2.11");
    putnl();
    return;
  }
  parse();
  cp = skip(cp);
  if (err == 2) {
    puts("? too deep");
    putnl();
    return;
  }
  if (err || *cp) {
    puts("? at '");
    puts(cp);
    putc('\'');
    putnl();
    return;
  }
  ovh = timed(2);
#ifndef SMALL
  tf = timed(0);
  f = fstk[0];
#endif
  tb = timed(1);
  if (err) {
    puts("? too deep");
    putnl();
    return;
  }
  tb = tb > ovh ? tb - ovh : 0;
#ifndef SMALL
  tf = tf > ovh ? tf - ovh : 0;
  bf = bf16_to_float(bstk[0]);
  puts("float32 ");
  put_float(f, 14);
  puts(" (");
  put_hex32(f);
  puts(")  ");
  put_time(tf);
  putnl();
  puts("bf16    ");
  put_float(bf, 14);
  puts(" (");
  puthex4(bstk[0]);
  puts(")      ");
  put_time(tb);
  puts("  off by ");
  put_float(bf - f, 0);
  putnl();
#else
  puts("bf16    ");
  put_bf16(bstk[0], 14);
  puts(" (");
  puthex4(bstk[0]);
  puts(")  ");
  put_time(tb);
  putnl();
#undef ovh
#endif
}

static void cmd_help(void)
{
  puts("help               this list");            putnl();
  puts("mr addr [len]      dump data RAM (hex, 16 bytes)"); putnl();
  puts("mw addr v [v ...]  write bytes");            putnl();
  puts("stack [depth]      SP and the bytes above it"); putnl();
  puts("free               data end, the limit, stack use and high water"); putnl();
#ifdef SMALL
  puts("calc expr          real arithmetic in bf16 on the FPU, timed"); putnl();
#else
  puts("calc expr          real arithmetic: bf16 on the FPU vs float32, timed"); putnl();
#endif
  puts("banner             See LISA");              putnl();
}

void main(void)
{
  unsigned char shown = 0;
#define p cp                            /* the parser's cursor serves as the command pointer: main's frame is under every chain */

  stack_paint();
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
#undef p
}

/* last in the data: the line right below the stack (see the top) */
struct gbuf g;
