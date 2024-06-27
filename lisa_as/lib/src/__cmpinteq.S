/*
================================================================================
Convert unsigned integer compare.  

asmsyntax=lisa
================================================================================
*/

    .segment .text

    .public __cmpinteq

__cmpinteq:
    cmp         2(sp)       // Compare LSB with LSB
    bnz         _ne
    swap        1(sp)       // Get MSB
    cmp         3(sp)       // Compare with MSB
    bnz         _ne

    // Add 1 to return address to skip the branch
    ldc         0           // NC/NZ means comparison is TRUE 
    br          _eq
_ne:
    ldc         1           // C/Z means comparison is FALSE 
_eq:
    swap        1(sp)       // Restore MSB/LSB
    ldz         c           // Set Z from the carry flag
    ads         2           // Remove 2nd int from stack
    ret

// vim:  sw=4 ts=4

