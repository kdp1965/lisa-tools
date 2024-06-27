/*
================================================================================
Perform unsigned division of uint8_t / uint8_t

asmsyntax=lisa
================================================================================
*/

    .segment .text

    .extern _div0_vec
    .public __udivcharchar

__udivcharchar:
    cpi       0             // Test for divide by zero
    bnz       _udivchcherr  // Jump to divide by zero handler if zero
    ads       -1            // Need one stack byte
    stax      0(sp)         // Save divisor
    ldi       0             // Initialize result
    swap      0(sp)         // Save zeroed result, get divisor
    swap      1(sp)         // Save divisor, get dividend
_udivchchloop:
    inx       0(sp)         // Increment the result
    sub       1(sp)         // Repeated subtract of divisor
    if        nc
    br        _udivchchloop // Loop until carry (borrow)
    dcx       0(sp)         // Last increment was not valido

    stax      1(sp)         // Save the remainder
    ldax      0(sp)         // Get the result
    ads       1(sp)
    ret

_udivchcherr:
    sra                     // Save return address
    jal       _div0_vec     // Jump to handle error
    lra                     // Load return, just in case
    ret

// vim:  sw=4 ts=4

