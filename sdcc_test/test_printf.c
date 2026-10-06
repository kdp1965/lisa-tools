/* printf through the SDCC library: compared against expected/test_printf.txt */
#include <stdio.h>
#include <string.h>

__sfr __at(0x210) UART_TX;
__sfr __at(0x211) UART_STATUS;

int putchar(int c)
{
  while (!(UART_STATUS & 2))
    ;
  UART_TX = c;
  return c;
}

char name[16];

int main(void)
{
  unsigned int i;
  long big = -123456;

  strcpy(name, "LISA");
  strcat(name, "!");
  printf("Hello from %s, len %u\n", name, (unsigned)strlen(name));
  for (i = 0; i < 3; i++)
    printf("i=%u sq=%u hex=%04x\n", i, i * i, i * 0x111);
  printf("big=%ld char=%c %d%%\n", big, 'Z', -42);
  printf("%d %d %d %d\n", -1, -10, -42, -300);
  printf("%ld %ld\n", -123456L, -1L);
  printf("%d %u %x\n", 42, 42, 42);
  printf("%d|%5d|%-5d|%05d\n", -7, -7, -7, -7);
  return 0;
}
