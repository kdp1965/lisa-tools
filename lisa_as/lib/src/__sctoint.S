/*
================================================================================
Convert signed Char to int (signed or unsigned).  

asmsyntax=lisa
================================================================================
*/

    .segment .text
    .public __sctoint

__sctoint:
    stax        1(sp)       // MSB of int is always at 1(sp).  Use it to save LSB
    ldi         0x80        // Prepare to test sign bit
    and         1(sp)       // Test if int8_t sign is negative
    ifte        z
    ldi         0           // Sign extend as positive value
    ldi         0xFF        // Sign extend as negative value
    swap        1(sp)       // Set MSB to zero, restore LSB
    ret

// vim:  sw=4 ts=4

