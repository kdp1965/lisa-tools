/*
================================================================================
Convert unsigned integer compare.  

asmsyntax=lisa
================================================================================
*/

    .segment .text

    .public __cmpintne

__cmpintne:
    cmp         2(sp)       // Compare LSB with LSB
    bnz         _ne
    swap        1(sp)       // Get MSB
    cmp         3(sp)       // Compare with MSB
    bnz         _ne
    ldc         1           // C/Z means FALSE comparison
    br          _eq

    // Add 1 to return address to skip the branch
_ne:
    ldc         0           // NC/NZ means TRUE comparison
_eq:
    swap        1(sp)       // Restore MSB/LSB
    ldz         c           // Load Z from C
    ads         2           // Remove 2nd int from stack
    ret

// vim:  sw=4 ts=4

