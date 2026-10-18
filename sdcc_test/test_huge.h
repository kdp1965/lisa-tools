/* test_huge.h - the functions of test_huge.c (common code), test_huge5.c
 * (--codeseg BANK5) and test_huge6.c (--codeseg BANK6), built with
 * --model-huge: every function is a banked call unless __nonbanked, with
 * nothing in the sources saying so.  See test_huge.c. */
unsigned char h5_byte(unsigned char x);
unsigned int  h5_sum(unsigned int a, unsigned int b);
long          h5_mul(long a, long b);
unsigned char h5_depth(unsigned char n);
typedef unsigned char (*fn_t)(unsigned char);                 /* 3 bytes in the huge model */
typedef unsigned char (*nbfn_t)(unsigned char) __nonbanked;   /* 2 bytes */
unsigned char h5_apply(fn_t f, unsigned char x);
unsigned char h5_apply_nb(nbfn_t f, unsigned char x);
unsigned char h6_byte(unsigned char x);
unsigned char h6_depth(unsigned char n);
unsigned int  h6_sum3(unsigned char a, unsigned int b, unsigned char c);
unsigned char common_add(unsigned char x);
unsigned char common_add_nb(unsigned char x) __nonbanked;
