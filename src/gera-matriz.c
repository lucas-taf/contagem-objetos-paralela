/*
 * gera-matriz.c - Gera matriz binaria pseudoaleatoria reprodutivel
 * (gerador congruente linear proprio => mesma matriz em Linux e macOS).
 * Uso: ./gera-matriz <linhas> <colunas> <densidade_%> <semente> > arquivo.txt
 */
#include <stdio.h>
#include <stdlib.h>

static unsigned long estado;

static unsigned long proximo_aleatorio(void)
{
    estado = (estado * 1103515245UL + 12345UL) & 0x7fffffffUL;
    return estado >> 4;
}

int main(int argc, char *argv[])
{
    long l, c, i, j;
    int dens;
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <linhas> <colunas> <densidade_%%> <semente>\n", argv[0]);
        return EXIT_FAILURE;
    }
    l = atol(argv[1]);
    c = atol(argv[2]);
    dens = atoi(argv[3]);
    estado = (unsigned long) atol(argv[4]);
    if (l <= 0 || c <= 0 || dens < 0 || dens > 100) {
        fprintf(stderr, "Parametros invalidos\n");
        return EXIT_FAILURE;
    }
    printf("%ld %ld\n", l, c);
    for (i = 0; i < l; i++) {
        for (j = 0; j < c; j++) {
            int v = (int) (proximo_aleatorio() % 100) < dens ? 1 : 0;
            putchar(v ? '1' : '0');
            putchar(j + 1 < c ? ' ' : '\n');
        }
    }
    return EXIT_SUCCESS;
}
