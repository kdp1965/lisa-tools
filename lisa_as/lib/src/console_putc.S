// ========================================================
// asmsyntax=lisa
//
// Introducing:           .{{{}}}}}}.
//    LISA               {{{{{}}}}}}}.
//                      {{{{  {{{{{}}}}
//                     }}}}} _   _ {{{{{
//                     }}}}  6   6  }}}}
//                    {{{{C    ^    {{{{
//                   }}}}}}\  '='  /}}}}}
//                   {{{{{{{;.___.;}}}}}}
//                    {{{{{{{)   (}}}}}}'
//              jgs    `""'"':   :'"'"'`
//                            `@` 
//  Image from:          
//     https://www.oocities.org/spunk1111/women.htm
//
//  File:  ASM implementation for console_putc
//
// ========================================================

#ifdef CONSOLE_UART2
#define  UART_STATUS          0x211
#define  UART_RXTX            0x210
#else
#define  UART_STATUS          0x211
#define  UART_RXTX            0x210
#endif

#define  UART_STATUS_TXEMPTY  2

   .section .text
   .public console_putc

console_putc:
   ads      -1
   stax     0(sp)
//   push_a

console_wait_tx:
   // Wait for UART TX to be ready
   lda      UART_STATUS
   andi     UART_STATUS_TXEMPTY
   bz       console_wait_tx

   ldax     0(sp)
   ads      1
//   pop_a
   sta      UART_RXTX
   ret

