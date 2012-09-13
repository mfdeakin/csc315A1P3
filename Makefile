
CC=gcc
CFLAGS=-g -Wall
LIBS=-lGL -lglut -lGLU

prog: main.o
	gcc -o prog main.o ${LIBS}