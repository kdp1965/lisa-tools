/*
================================================================================
Perform signed / unsigned division of int8_t / int8_t

asmsyntax=lisa
================================================================================
*/

    .segment .text

    .extern  _div0_vec
    .public __udivchch
    .public __sdivchch

__sdivchch:
    amode     2             // Configure for signed operation
__udivchch:                 // Default amode is zero ... unsigned
    sra                     // Save the return address
    tax                     // Put 8-bit dividend in ix[7:0]
    ldax      0(sp)         // Get 8-bit divisor
    ifte      z             // Test for divide by zero
    jal       _div0_vec     // Jump to the divide by zero vector
    div       cc            // Perform divison
    lra                     // Restore RA
    ret

// vim:  sw=4 ts=4

