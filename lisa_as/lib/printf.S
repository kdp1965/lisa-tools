/*
==========================================================
Printf implementation for LISA

asmsyntax=lisa
==========================================================
*/

  .segment .text
  .public  printf

printf:
  sra           // Save RA to stack

  lra           // Resore return address
  ret


