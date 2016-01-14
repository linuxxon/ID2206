SRC	= malloc.h malloc.c tstalgorithms.c  \
	  tstextreme.c tstmalloc.c  tstmemory.c tstrealloc.c tstmerge.o \
	  best-case.c average-case.c worst-case.c test-env.h

## Use STDMALLOC as to switch between standard malloc implementation and custom
ifeq ($(STDMALLOC), true)
MALLOC_OBJ = 
else
MALLOC_OBJ = malloc.o
endif

OBJ	= $(MALLOC_OBJ) tstalgorithms.o  tstcrash_simple.o\
	  tstextreme.o tstmalloc.o  tstmemory.o tstrealloc.o tstmerge.o \
	  best-case.o average-case.o worst-case.o

BIN	= t0 t1 t2 t3 t4 t5

STRATEGY ?= 1
STRAT = -DSTRATEGY=$(STRATEGY)
CFLAGS	= -Wall -ansi -O4 $(STRAT)

CC	= gcc 

.PHONY: all clean cleanall $(BIN)

all: $(BIN)
	exec ./RUN_TESTS

best-case: best-case.o $(MALLOC_OBJ)
	$(CC) $(CFLAGS) -o $@ best-case.o $(MALLOC_OBJ)

average-case: average-case.o $(MALLOC_OBJ)
	$(CC) $(CFLAGS) -o $@ average-case.o $(MALLOC_OBJ)

worst-case: worst-case.o $(MALLOC_OBJ)
	$(CC) $(CFLAGS) -o $@ worst-case.o $(MALLOC_OBJ)

t0: tstmerge.o $(MALLOC_OBJ) $(X)
	$(CC) $(CFLAGS) -o  $@ tstmerge.o $(MALLOC_OBJ) $(X)

t1: tstalgorithms.o $(MALLOC_OBJ) $(X)
	$(CC) $(CFLAGS) -o  $@ tstalgorithms.o $(MALLOC_OBJ) $(X)

t2: tstextreme.o $(MALLOC_OBJ) $(X)
	$(CC) $(CFLAGS) -o $@  tstextreme.o $(MALLOC_OBJ) $(X)

t3: tst$(MALLOC_OBJ)  $(MALLOC_OBJ) $(X)
	$(CC) $(CFLAGS) -o $@ tst$(MALLOC_OBJ)  $(MALLOC_OBJ) $(X) 

t4: tstmemory.o $(MALLOC_OBJ) $(X)
	$(CC) $(CFLAGS) -o $@ tstmemory.o $(MALLOC_OBJ) $(X)

t5: tstrealloc.o $(MALLOC_OBJ) $(X)
	$(CC) $(CFLAGS) -o $@ tstrealloc.o $(MALLOC_OBJ) $(X)


clean:
	\rm -f $(BIN) $(OBJ) core

cleanall: clean
	\rm -f *~

