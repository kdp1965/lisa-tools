
PROGRAM = lisa_cc
CFLAGS  = -Wall -Wno-strict-aliasing -std=gnu11 -g -I. -O0 -DSTD_P16CC
ALLSRCS = $(wildcard *.c)
SRCS    = $(filter-out utiltest.c, $(ALLSRCS))
OBJTMP  = $(SRCS:.c=.o)
 
OBJS    = $(patsubst %.o,obj/%.o,$(OBJTMP))
#OBJS    = cpp.o debug.o dict.o gen.o lex.o vector.o parse.o buffer.o map.o \
#         error.o path.o file.o set.o encoding.o opt_lisa.o
TESTS := $(patsubst %.c,%.bin,$(filter-out test/testmain.c,$(wildcard test/*.c)))
ECC    = ./$(PROGRAM)

override CFLAGS += -DBUILD_DIR='"$(shell pwd)"'

all: lisa_cc

init:
	@mkdir -p obj

$(PROGRAM): init lisacc.h $(OBJS)
	cc -o $@ $(OBJS) -lm $(LDFLAGS)

obj/%.o: %.c
	cc $(CFLAGS) -o $@ -c $<

$(OBJS) utiltest.o main.o: lisacc.h keyword.inc

utiltest: lisacc.h utiltest.o $(OBJS)
	cc -o $@ utiltest.o $(OBJS) $(LDFLAGS)

test/%.o: test/%.c $(ECC)
	$(ECC) -w -o $@ -c $<

test/%.bin: test/%.o test/testmain.o
	cc -o $@ $< test/testmain.o $(LDFLAGS)

self: $(PROGRAM) cleanobj
	$(MAKE) CC=$(ECC) CFLAGS= lisacc

test: $(PROGRAM) $(TESTS)
	$(MAKE) CC=$(ECC) CFLAGS= utiltest
	./utiltest
	./test/ast.sh
	./test/negative.py
	$(MAKE) runtests

runtests:
	@for test in $(TESTS); do  \
	    ./$$test || exit;      \
	done

stage1:
	$(MAKE) cleanobj
	[ -f $(PROGRAM) ] || $(MAKE) $(PROGRAM)
	mv $(PROGRAM) stage1

stage2: stage1
	$(MAKE) cleanobj
	$(MAKE) CC=./stage1 ECC=./stage1 CFLAGS= lisacc
	mv $(PROGRAM) stage2

stage3: stage2
	$(MAKE) cleanobj
	$(MAKE) CC=./stage2 ECC=./stage2 CFLAGS= lisacc
	mv $(PROGRAM) stage3

# Compile and run the tests with the default compiler.
testtest:
	$(MAKE) clean
	$(MAKE) $(TESTS)
	$(MAKE) runtests

fulltest: testtest
	$(MAKE) stage1
	$(MAKE) CC=./stage1 ECC=./stage1 CFLAGS= test
	$(MAKE) stage2
	$(MAKE) CC=./stage2 ECC=./stage2 CFLAGS= test
	$(MAKE) stage3
	cmp stage2 stage3

clean: cleanobj
	rm -f $(PROGRAM) stage?

cleanobj:
	rm -rf obj *.s test/*.o test/*.bin utiltest

.PHONY: clean cleanobj test runtests fulltest self all
