/*
================================================================================
Perform signed multiplication of int16_t * int8_t

asmsyntax=lisa
================================================================================
*/

    .segment .text

    .public __smulintchar

__smulintchar:
    amode     2             // Set signed math mode
    swap      0(sp)         // Save uint8_t, get LSB of uint16_t
    mul       0(sp)         // Multipliy the LSB of uint16_t
    swap      0(sp)         // Get uint8_t, save LSB of multiply
    mulu      1(sp)         // Multiply the MSB
    stax      1(sp)         // Save the MSB of multiply
    ldax      0(sp)         // Get LSB of the product
    amode     0             // Restore default math mode
    ret

// vim:  sw=4 ts=4

