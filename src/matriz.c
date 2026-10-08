/*
 * matriz.c - Leitura da matriz e medicao de tempo (ANSI C89 + POSIX)
 */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include "matriz.h"

const int VIZ_DL[8] = { -1, -1, -1,  0, 0,  1, 1, 1 };
const int VIZ_DC[8] = { -1,  0,  1, -1, 1, -1, 0, 1 };

/* Le o proximo inteiro nao negativo do arquivo (rapido, sem fscanf). */
static int ler_inteiro(FILE *f, long *valor)
{
    int ch;
    long v = 0;
    do {
        ch = getc(f);
    } while (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t' || ch == ',');
    if (ch < '0' || ch > '9') {
        return -1;
    }
    while (ch >= '0' && ch <= '9') {
        v = v * 10 + (ch - '0');
        if (v > INT_MAX) {
            return -1;
        }
        ch = getc(f);
    }
    *valor = v;
    return 0;
}

int matriz_carregar(const char *caminho, Matriz *m)
{
    FILE *f;
    long l, c, v, i, n;

    m->linhas = 0;
    m->colunas = 0;
    m->dados = NULL;

    f = fopen(caminho, "r");
    if (f == NULL) {
        perror(caminho);
        return -1;
    }
    if (ler_inteiro(f, &l) != 0 || ler_inteiro(f, &c) != 0 || l <= 0 || c <= 0) {
        fprintf(stderr, "%s: cabecalho invalido (esperado: LINHAS COLUNAS)\n", caminho);
        fclose(f);
        return -1;
    }
    if (l > (INT_MAX - 1) / c) {
        fprintf(stderr, "%s: matriz grande demais\n", caminho);
        fclose(f);
        return -1;
    }
    n = l * c;
    m->dados = (unsigned char *) malloc((size_t) n);
    if (m->dados == NULL) {
        fprintf(stderr, "%s: memoria insuficiente\n", caminho);
        fclose(f);
        return -1;
    }
    for (i = 0; i < n; i++) {
        if (ler_inteiro(f, &v) != 0 || (v != 0 && v != 1)) {
            fprintf(stderr, "%s: valor invalido ou ausente na posicao %ld\n", caminho, i);
            free(m->dados);
            m->dados = NULL;
            fclose(f);
            return -1;
        }
        m->dados[i] = (unsigned char) v;
    }
    fclose(f);
    m->linhas = (int) l;
    m->colunas = (int) c;
    return 0;
}

void matriz_liberar(Matriz *m)
{
    free(m->dados);
    m->dados = NULL;
    m->linhas = 0;
    m->colunas = 0;
}

double tempo_ms(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        return 0.0;
    }
    return (double) tv.tv_sec * 1000.0 + (double) tv.tv_usec / 1000.0;
}
