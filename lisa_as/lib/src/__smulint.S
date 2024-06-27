/*
================================================================================
Perform signed multiplication of uint16_t * uint16_t

NOTE:  Keeps only the 16 LSBs

asmsyntax=lisa
================================================================================
*/

    .segment .text

    .public __smulint
    .extern __umulint

__smulint:
    sra                     // Save return address on stack
    amode     2             // Set signed math mode
    jal       __umulint     // Call 16-bit integer multiply
    amode     0             // Restore default amode
    lra                     // Restore return address
    ret

// vim:  sw=4 ts=4

