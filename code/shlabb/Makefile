#Makefile for miniShell

OUTPUT = minish
OBJS = minish.o digenv.o
CC   = gcc
CFLAGS = -c -std=c99 -D_POSIX_C_SOURCE
LFLAGS = 
PREPROC =

.PHONY : all sigdet

CFLAGS += $(PREPROC)


all : clean $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $(OUTPUT)

sigdet : PREPROC = -DSIGNALDETECTION 
sigdet : all
	
minish.o : minish.c digenv.h
	$(CC) $(CFLAGS) minish.c

digenv.o : digenv.c
	$(CC) $(CFLAGS) digenv.c

clean:
	-rm *.o
	-rm $(OUTPUT)
