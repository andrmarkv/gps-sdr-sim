# Makefile for Linux etc.

.PHONY: all clean time
all: gps-sdr-sim

SHELL=/bin/bash
CC=gcc
CFLAGS=-O3 -Wall
LDFLAGS=-lm -lpthread

gps-sdr-sim: gpssim.o
	${CC} ${CFLAGS} getopt.c cqueue.c path_generator.c gpssim.c ${LDFLAGS} -o $@

clean:
	rm -f gpssim.o gps-sdr-sim *.bin
#	mkfifo gpssim.bin

