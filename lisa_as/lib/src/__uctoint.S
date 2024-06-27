/*
================================================================================
Convert unsigned Char to int (signed or unsigned).  

asmsyntax=lisa
================================================================================
*/

    .segment .text

    .public __uctoint

__uctoint:
    stax        1(sp)       // MSB of int is always at 1(sp).  Use it to save LSB
    andi        0           // Prepare to set MSB to zero
    swap        1(sp)       // Set MSB to zero, restore LSB
    ret

// vim:  sw=4 ts=4

