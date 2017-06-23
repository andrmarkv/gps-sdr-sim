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
	rm -f gpssim.o gps-sdr-sim 
	#rm *.bin
	#mkfifo gpssim.bin

run_hack:
	day=$(shell date +%j)
	year=$(shell date +%Y)
	y=$(shell date +%y)
	wget ftp://cddis.gsfc.nasa.gov/gnss/data/daily/${year}/${day}/${y}n/brdc${day}0.${y}n.Z
