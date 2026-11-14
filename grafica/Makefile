CC = gcc
CPPFLAGS = -I.
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes -Werror
LDLIBS = -lcomctl32 -lgdi32 -luser32 -lm
SHELL := cmd.exe
.SHELLFLAGS := /C

.PHONY: all run test clean
all: build/Artemis-SLS-Nativo.exe

build:
	@if not exist build mkdir build

build/main.o: main.c fisica.h escena.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
build/fisica.o: fisica.c fisica.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
build/escena.o: escena.c escena.h fisica.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/Artemis-SLS-Nativo.exe: build/main.o build/fisica.o build/escena.o
	$(CC) $(CFLAGS) $^ -o $@ -mwindows $(LDLIBS)

build/pruebas.exe: tests/pruebas.c fisica.c fisica.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/pruebas.c fisica.c -o $@ -lm

run: all
	.\build\Artemis-SLS-Nativo.exe

test: build/pruebas.exe
	.\build\pruebas.exe

clean:
	@if exist build\*.o del /Q build\*.o
	@if exist build\Artemis-SLS-Nativo.exe del /Q build\Artemis-SLS-Nativo.exe
	@if exist build\pruebas.exe del /Q build\pruebas.exe
	@if exist build rmdir build
