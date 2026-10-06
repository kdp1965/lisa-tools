/*
================================================================================
bringup.c:  Test program for the lisa core
================================================================================
*/
#include <stdint.h>
#include <stdio.h>
#include "bringup.h"

uint8_t gDebounce = 255;
uint16_t access gCount;

/*
================================================================================
Initialize the I/O
================================================================================
*/
void init(void)
{
  //TIMER_PRE_DIV = 50000000/50000;
  // Configure pre-divider for 1 microsecond
  TIMER1_CTRL = 0;
  TIMER1_PRE_DIVL = 0xf0;
  TIMER1_PRE_DIVH = 0x55;

  // Tick every 250 ms
  TIMER1_DIVL = 250;

  // Enable the timer
  TIMER1_CTRL = 1;
  TIMER1_TICK = 0;

  // Configure the PORTA port
  PORTB     = 0x4F;
}

/*
================================================================================
print_lisa:  
================================================================================
*/
void print_lisa(void)
{
  puts_c("");  
  puts_c("     .{{{}}}}}}.");
  puts_c("    {{{{{}}}}}}}.");
  puts_c("   {{{{  {{{{{}}}}");
  puts_c("  }}}}} _   _ {{{{{");
  puts_c("  }}}}  6   6  }}}}");
  puts_c(" {{{{C    ^    {{{{");
  puts_c("}}}}}}\\  '='  /}}}}}");
  puts_c("{{{{{{{;.___.;}}}}}}");
  puts_c(" {{{{{{{)   (}}}}}}'");
  puts_c("  ''\"''':   :''''''");
  puts_c("  jgs    `@` ");
  puts_c("\r\nHello from TT07 LISA!");
}

/*
================================================================================
main:  
================================================================================
*/
void main(void)
{
  uint8_t   rxd;
  uint8_t   sum;
  uint8_t   x;
  uint8_t   y;
  uint8_t   count;
  uint8_t   count2;

  uint16_t  count3;
  uint8_t  *ptr;

  // Initialize the timer and UART
  init();

  count = 0;
  count2 = 0;
  count3 = 0;
  sum   = 0;
  while (1)
  {
    // Test if the timer expired
    if (TIMER1_CTRL & TIMER_ROLLOVER)
    {
       count = count + 1;
       if (count == 4)
       {
         count = 0;
         PORTB ^= 0x8;
       }
       count2++;
       if (count2 == 7)
       {
         count2 = 0;
         PORTB ^= 0x40;
       }
    }

    count3++;

    // Test for received data and echo it back
    if (UART_STATUS & 0x01)
    {
      // Read the RX data
      rxd = UART_RXTX;

      // Write TX data
      UART_RXTX = rxd;

      sum += rxd;

      if (rxd == 0x0d)
      {
        while ((UART_STATUS & 0x02) == 0)
          ;
        UART_RXTX = 0x0a;
      }

      // Test for hello request
      if (rxd == '?')
        print_lisa();

      // Test for sum request
      if (rxd == 's')
        printf("\r\nCount: %d sum: %d\r\n", count3, sum);

      // Test for cache test request
      if (rxd == 'c')
      {
        ptr = (uint8_t *) 0x200;
        for (x = 0; x < 4; x++)
        {
          for (y = 0; y < 127; y++)
            *ptr++ = x++;
        }
      }
    }
  }
}

// vim: sw=2 ts=2
