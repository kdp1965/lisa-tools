/* strings in code space, globals, a loop: compared against expected/hello.txt */
#include "harness.h"

const char msg[] = "Hello from LISA via SDCC\n";
unsigned char buf[8];
unsigned int counter;

int main(void)
{
  unsigned char i;

  puts(msg);
  for (i = 0; i < 8; i++)
    buf[i] = i * 3;
  for (i = 0; i < 8; i++)
    counter += buf[i];
  puts("sum=");
  puthex(counter);
  putc('\n');
  return 0;
}
