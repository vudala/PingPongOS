#!/bin/bash

CC=gcc
FLAGS=-Wall -g
LIBS=-lm
SRC=$(wildcard *.c)
OBJS=$(subst .c,.o,$(SRC))
OUTPUT=program
RM=rm -f

all: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(OUTPUT) $(LIBS)

%.o: %.c
	$(CC) $(FLAGS) -c $<

run:
	./program

clean:
	@$(RM) $(OBJS)

purge: clean
	@$(RM) $(OUTPUT)