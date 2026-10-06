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
//  ASCII Art from:          
//     https://www.oocities.org/spunk1111/women.htm
//
//  File:  ASM implementation for printf
//
// ========================================================

#define  UART_STATUS          0x211
#define  UART_RXTX            0x210
#define  UART_STATUS_TXEMPTY  2

#define  PUTS_LF              0x0a
#define  PUTS_CR              0x0d
#define  ASCII_PERCENT        0x25

#define  IX_OFF               2

   .section .text

   .extern  console_putc
   .public printf

printf:
   sra
   ads      -2

printf_next:
   // Get the next byte
   call_ix
   adx      1
   cpi      0
   bz       printf_exit

   // Test for format specifier
   cpi      ASCII_PERCENT
   iftt     ne
   jal      console_putc
   br       printf_next

   // =======================================
   // Format specifier found!  Process it
   // =======================================
   // Get next string byte
   call_ix
   adx      1

   // Test for end of string
   cpi      0
   bz       printf_exit

   // Test if printing % (i.e. it was %%)
   cpi      ASCII_PERCENT
   iftt     ne
   jal      console_putc
   br       printf_next

   // =============================================
   // Okay, it's a real specifier.  First save
   // IX to keep track of our place in the format
   // string.  We will use IX to get the next arg
   // =============================================
   


printf_exit:
   // Exit
   ads      2
   lra
   ret

// vim: syntax=lisa et sw=4 ts=4

