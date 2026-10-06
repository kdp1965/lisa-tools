// ========================================================
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
//  File:  ASM implementation for puts_c
//
// ========================================================
#define  UART_STATUS          0x211
#define  UART_RXTX            0x210
#define  UART_STATUS_TXEMPTY  2

#define  PUTS_LF              0x0a
#define  PUTS_CR              0x0d

   .section .text

   .extern  console_putc

   .public puts_c

puts_c:
   sra
   ads      -1

puts_next:
   // Get the next byte
   call_ix
   adx      1
   cpi      0
   bz       puts_done

   // Test for compressed text
   btst     7
   bz       puts_run
   jal      console_putc
   br       puts_next

puts_run:
   andi     0x7F
   sta      0(sp)
   call_ix
   adx      1

puts_run_loop:
   jal      console_putc
   dcx      0(sp)
   bz       puts_next
   br       puts_run_loop

puts_done:
   // Send \n
   ldi      PUTS_LF
   jal      console_putc

   // Send \r
   ldi      PUTS_CR
   jal      console_putc

   // Exit
   ads      1
   lra
   ret

// vim: syntax=lisa et sw=3 ts=3
