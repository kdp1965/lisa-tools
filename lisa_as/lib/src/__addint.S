/*
================================================================================
Perform 16-bit addition

asmsyntax=lisa
================================================================================
*/

    .segment .text

    .public __addint

__addint:
    ldc       0             // Ensure cflag is zero
    add       2(sp)         // Add LSBs
    sta       2(sp)         // Save LSB
    lda       1(sp)         // Get MSB of 1st num
    add       3(sp)         // Add MSBs
    sta       3(sp)         // Save MSB
    lda       2(sp)         // Get LSB of result
    ads       2             // Pop 2nd number from stack
    ret

// vim:  sw=4 ts=4

