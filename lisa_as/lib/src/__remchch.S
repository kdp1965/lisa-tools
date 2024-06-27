/*
================================================================================
Perform signed / unsigned remainder of int8_t / int8_t

asmsyntax=lisa
================================================================================
*/

    .segment .text

    .extern  _div0_vec
    .public __uremchch
    .public __sremchch

__sremchch:
    amode     2             // Configure for signed operation
__uremchch:                 // Default amode is zero ... unsigned
    tax                     // Put 8-bit dividend in ix[7:0]
    ldax      0(sp)         // Get LSB of divisor
    bnz       __doremchch
    sra                     // Save the return address
    jal       _div0_vec     // Jump to the divide by zero vector
    lra                     // Restore RA
    br        __remintexit

__doremchch:
    rem       cc            // Perform remainder operation
__remintexit:
    amode     0             // Restore default unsigned mode
    ret

// vim:  sw=4 ts=4

