/*
==========================================================
C Runtime or LISA architecture

asmsyntax=lisa
==========================================================
*/

    .segment    .text.vec
    .extern     main
    .extern     _stack_start
    .extern     _stack_end
    .extern     _sdata
    .extern     _edata
    .extern     _bss_start
    .extern     _bss_end
    .extern     _virt_regs
    .extern     data_sram_origin
    .public     reset_vec_noint
    .extern     porta_isr
    // The reset vector
reset_vec_noint:
    // ==============================================================
    // Initialize the .data section (initialized C globals, etc.)
    // ==============================================================
_init:
    ldx     data_sram_origin
    xchg    sp
    ldx     _sdata
_data_loop:
    xchg    ra              // Preserve IX
    ldx     _edata          // Get end of initialized data
    xchg    ra              // Restore IX
    cpx     ra              // Test if IX == _edata
    if      eq
    br      _init_bss
    call_ix                 // Get next initializer bytes
    stax    0(sp)           // Save next byte
    ads     1               // Increment SP SRAM pointer
    br      _data_loop      // Branch to copy all data

    // ==============================================================
    // Initialize the BSS section to zero
    // ==============================================================
_init_bss:
    ldx     _bss_end        // Get .bss end address
    xchg    ra              // Save in ra for comparison
    ldx     _bss_start      // Get .bss start address
    ldi     0               // Prepare to write Zero to memory
_bss_loop:
    cpx     ra              // Compare with end address
    if      eq              // Test if at the end of BSS 
    br      _init_stack     // Jump to init stack
    stax    0(ix)           // Write next zero to RAM
    adx     1               // Increment ix
    br      _bss_loop       // Branch to zero all BSS RAM

    // ==============================================================
    // Initialize the stack with 0xA5
    // ==============================================================
_init_stack:
    ldx     _stack_start
    xchg    sp
    ads     -1              // Point to first byte of top of stack

    ldx     _virt_regs      // Point to virtual register space
    ldi     0               // A=0 too
    jal     main            // Now jump to C 'main'

// vim: sw=4 ts=4
