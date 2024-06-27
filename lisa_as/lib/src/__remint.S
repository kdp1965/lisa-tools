/*
================================================================================
Perform unsigned remainder of uint16_t / uint16_t

asmsyntax=lisa
================================================================================
*/

    .segment .text

    .extern  _div0_vec
    .public __uremint
    .public __sremint

__sremint:
    amode     2             // Configure for signed operation
__uremint:                  // Default amode is zero ... unsigned
    sra                     // Save the return address ... we will destroy it
    ads       2             // Point to dividend
    lddiv                   // Load dividend into ra/ix
    ads       2             // Pop dividend from stack
    ldax      0(sp)         // Get LSB of divisor
    or        1(sp)         // Test for divide by zero
    bnz       __doremint
    ads       -4            // Put stack back to our sra
    jal       _div0_vec     // Jump to the divide by zero vector
    br        __remintexit

__doremint:
    ldax      0(sp)         // Get LSB of divisor
    rem       ic            // Perform divison
    ads       -4            // Point to our saved RA
__remintexit:
    lra                     // Load return address
    ads       2             // Restore to our result
    amode     0             // Restore default unsigned mode
    ret

// vim:  sw=4 ts=4

