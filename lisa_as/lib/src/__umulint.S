/*
================================================================================
Perform unsigned multiplication of uint16_t * uint16_t

NOTE:  Keeps only the 16 LSBs

asmsyntax=lisa
================================================================================
*/

    .segment .text

    .public __umulint

__umulint:
    // LSB1 * MSB2
    stax      0(sp)         // Save nLow
    mul       3(sp)         // Multiply the nLow and mHigh
    stax      3(sp)         // Save nLow * nHigh

    // LSB2 * MSB1
    ldax      2(sp)         // Get mLow
    mul       1(sp)         // Multiply mLow and nHigh
    ldc       0             // Set cflag to zero
    add       3(sp)         // Add partial products
    stax      3(sp)         // Save partial products

    // LSB1 * LSB2
    ldax      0(sp)         // Get nLow
    mulu      2(sp)         // Multiply nLow * mLow
    ldc       0             // Set cflag to zero
    add       3(sp)         // Add MSB of product to result
    ldax      0(sp)         // Get nLow
    mulu      2(sp)         // Multiply nLow * mLow

    ret

// vim:  sw=4 ts=4

