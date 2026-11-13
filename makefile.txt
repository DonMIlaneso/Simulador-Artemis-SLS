# Makefile - Simulador Artemis: ascenso del SLS
#   make        compila
#   make run    compila y ejecuta
#   make clean  borra los objetos y el ejecutable

CC     = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
LIBS   = -lm
OBJS   = main.o artemis.o validaciones.o

ifeq ($(OS),Windows_NT)
    PROGRAMA = artemis.exe
    BORRAR   = del /Q
    EJECUTAR = artemis.exe
else
    PROGRAMA = artemis
    BORRAR   = rm -f
    EJECUTAR = ./artemis
endif

all: $(PROGRAMA)

$(PROGRAMA): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROGRAMA) $(OBJS) $(LIBS)

main.o: main.c artemis.h validaciones.h
	$(CC) $(CFLAGS) -c main.c

artemis.o: artemis.c artemis.h validaciones.h
	$(CC) $(CFLAGS) -c artemis.c

validaciones.o: validaciones.c validaciones.h
	$(CC) $(CFLAGS) -c validaciones.c

run: $(PROGRAMA)
	$(EJECUTAR)

clean:
	$(BORRAR) $(OBJS) $(PROGRAMA)

.PHONY: all run clean