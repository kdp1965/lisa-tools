/* strings and pointer tables in code space, strlen, printf %s */
#include "harness.h"
#include <string.h>
int printf(const char *fmt, ...);
int putchar(int c) { putc(c); return c; }
static const char *const tab[] = { "one", "three", "fifteen", 0 };
const char msg[] = "hello";
static char buf[24];
static char ram[8] = "RAM str";
static char hi[2] = "\x80";                      /* above every ASCII byte: unsigned */
int main(void)
{
  const char *const *p;
  unsigned char n = 0;
  char *r;
  CHECK(1, strlen(msg) == 5);
  CHECK(2, strlen(tab[0]) == 3 && strlen(tab[1]) == 5 && strlen(tab[2]) == 7);
  for (p = tab; *p; p++) n++;
  CHECK(3, n == 3);
  CHECK(4, tab[1][2] == 'r' && *tab[2] == 'f');
  /* the assembly memcpy / memset / strcpy / strlen (the .s in device/lib/lisa):
     sources in code space and in RAM, empty strings, n == 0, the returns */
  CHECK(5, strlen(ram) == 7 && strlen("") == 0 && strlen(ram + 7) == 0);
  r = memset(buf, 'x', sizeof buf);
  CHECK(6, r == buf && buf[0] == 'x' && buf[23] == 'x');
  r = memset(buf, 0, 0);
  CHECK(7, r == buf && buf[0] == 'x');
  r = memcpy(buf, msg, 6);                      /* from code space, with the NUL */
  CHECK(8, r == buf && buf[0] == 'h' && buf[5] == 0 && buf[6] == 'x' && strlen(buf) == 5);
  r = memcpy(buf + 8, ram, 8);                  /* from RAM */
  CHECK(9, r == buf + 8 && buf[8] == 'R' && buf[14] == 'r' && buf[15] == 0 && buf[16] == 'x');
  r = memcpy(buf, ram, 0);
  CHECK(10, r == buf && buf[0] == 'h');
  r = strcpy(buf, tab[2]);                      /* from code space */
  CHECK(11, r == buf && strlen(buf) == 7 && buf[6] == 'n' && buf[7] == 0 && buf[8] == 'R');
  r = strcpy(buf + 16, ram);                    /* from RAM */
  CHECK(12, r == buf + 16 && buf[16] == 'R' && buf[22] == 'r' && buf[23] == 0);
  r = strcpy(buf, "");
  CHECK(13, r == buf && buf[0] == 0 && buf[1] == 'i');
  memset(buf, 0, 300 - 300);
  CHECK(14, buf[1] == 'i' && strcmp(buf + 16, ram) == 0 && strcmp(buf + 16, "RAM st") > 0);
  /* strcmp (strcmp.s): both arguments in RAM or in code space, the sign when
     one string ends (the TT07 cmp's borrow on a 0 operand), the unsigned
     ordering, the exact difference; buf is "" and buf + 16 "RAM str" here */
  CHECK(15, strcmp(ram, buf + 16) == 0 && strcmp(ram, "RAM str") == 0 && strcmp("RAM str", ram) == 0 && strcmp(msg, "hello") == 0);
  CHECK(16, strcmp(buf + 16, "RAM st") > 0 && strcmp("RAM st", buf + 16) < 0 && strcmp(ram, buf) > 0 && strcmp(buf, ram) < 0);
  CHECK(17, strcmp(buf + 16, "RAM su") < 0 && strcmp("RAM su", buf + 16) > 0 && strcmp(tab[0], tab[1]) < 0 && strcmp(tab[2], tab[0]) < 0);
  CHECK(18, strcmp("", "") == 0 && strcmp(buf, "") == 0 && strcmp("", ram) < 0 && strcmp("b", "a") == 1 && strcmp("a", "b") == -1);
  CHECK(19, strcmp(hi, "a") == 31 && strcmp("a", hi) == -31 && strcmp(hi, ram) > 0 && strcmp(ram, hi) < 0);
  /* memmove (C): its result, with its d homed in dst's slot once (SDCC's
     memory suite); a copy from code space, one backward, one forward */
  r = memmove(buf, "abcdefgh", 9);
  CHECK(20, r == buf && buf[7] == 'h' && memmove(buf + 1, buf, 7) == buf + 1 && buf[1] == 'a' && buf[7] == 'g' &&
            memmove(buf, buf + 1, 8) == buf && buf[0] == 'a' && buf[6] == 'g' && buf[7] == 0);
  puts("["); puts(tab[1]); puts("]\n");
  printf("[%s]\n", msg);
  printf("[%s]\n", tab[2]);
  printf("[%5s|%-5s]\n", "ab", "cd");
  for (p = tab; *p; p++) printf("%s\n", *p);
  DONE();
  return 0;
}
