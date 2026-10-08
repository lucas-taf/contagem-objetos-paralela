CC      ?= cc
CFLAGS  = -std=c89 -Wall -Wextra -pedantic -O2
LDFLAGS = -pthread

all: conta-objetos-sequencial conta-objetos-paralelo gera-matriz

conta-objetos-sequencial: src/conta-objetos-sequencial.c src/matriz.c src/matriz.h
	$(CC) $(CFLAGS) src/conta-objetos-sequencial.c src/matriz.c -o $@

conta-objetos-paralelo: src/conta-objetos-paralelo.c src/matriz.c src/matriz.h
	$(CC) $(CFLAGS) -pthread src/conta-objetos-paralelo.c src/matriz.c -o $@ $(LDFLAGS)

gera-matriz: src/gera-matriz.c
	$(CC) $(CFLAGS) src/gera-matriz.c -o $@

test: all
	./scripts/testa.sh

bench: all
	./scripts/benchmark.sh

clean:
	rm -f conta-objetos-sequencial conta-objetos-paralelo gera-matriz

.PHONY: all test bench clean
