/*
 * conta-objetos-sequencial.c
 * Versao SEQUENCIAL de referencia: contagem de componentes conexos com
 * conectividade 8 por flood fill ITERATIVO (pilha explicita, sem recursao).
 *
 * Uso: ./conta-objetos-sequencial <arquivo_matriz>
 * Padrao: ANSI C (C89/C90)
 */
#include <stdio.h>
#include <stdlib.h>
#include "matriz.h"

/* Pilha dinamica de indices lineares de celulas. */
typedef struct {
    int *itens;
    long topo;
    long capacidade;
} Pilha;

static int pilha_empilhar(Pilha *p, int valor)
{
    if (p->topo == p->capacidade) {
        long nova = p->capacidade * 2;
        int *tmp = (int *) realloc(p->itens, (size_t) nova * sizeof(int));
        if (tmp == NULL) {
            return -1;
        }
        p->itens = tmp;
        p->capacidade = nova;
    }
    p->itens[p->topo++] = valor;
    return 0;
}

/* Retorna o numero de objetos ou -1 em caso de falta de memoria. */
static long contar_objetos(const Matriz *m)
{
    long n = (long) m->linhas * m->colunas;
    long i, objetos = 0;
    unsigned char *visitado;
    Pilha pilha;
    int k;

    visitado = (unsigned char *) calloc((size_t) n, 1);
    pilha.capacidade = 1024;
    pilha.topo = 0;
    pilha.itens = (int *) malloc((size_t) pilha.capacidade * sizeof(int));
    if (visitado == NULL || pilha.itens == NULL) {
        free(visitado);
        free(pilha.itens);
        return -1;
    }

    for (i = 0; i < n; i++) {
        if (m->dados[i] == 0 || visitado[i]) {
            continue;
        }
        /* Nova semente: novo objeto. Flood fill a partir dela. */
        objetos++;
        visitado[i] = 1;
        pilha.topo = 0;
        if (pilha_empilhar(&pilha, (int) i) != 0) {
            objetos = -1;
            break;
        }
        while (pilha.topo > 0) {
            int cel = pilha.itens[--pilha.topo];
            int l = cel / m->colunas;
            int c = cel % m->colunas;
            for (k = 0; k < 8; k++) {
                int nl = l + VIZ_DL[k];
                int nc = c + VIZ_DC[k];
                int viz;
                if (nl < 0 || nl >= m->linhas || nc < 0 || nc >= m->colunas) {
                    continue;
                }
                viz = nl * m->colunas + nc;
                if (m->dados[viz] && !visitado[viz]) {
                    visitado[viz] = 1;
                    if (pilha_empilhar(&pilha, viz) != 0) {
                        free(visitado);
                        free(pilha.itens);
                        return -1;
                    }
                }
            }
        }
    }
    free(visitado);
    free(pilha.itens);
    return objetos;
}

int main(int argc, char *argv[])
{
    Matriz m;
    long objetos;
    double t0, t1;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo_matriz>\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (matriz_carregar(argv[1], &m) != 0) {
        return EXIT_FAILURE;
    }

    t0 = tempo_ms();                 /* mede somente o processamento */
    objetos = contar_objetos(&m);
    t1 = tempo_ms();

    if (objetos < 0) {
        fprintf(stderr, "Erro: memoria insuficiente\n");
        matriz_liberar(&m);
        return EXIT_FAILURE;
    }
    printf("Versao: sequencial\n");
    printf("Dimensoes: %d x %d\n", m.linhas, m.colunas);
    printf("Objetos: %ld\n", objetos);
    printf("Tempo_ms: %.3f\n", t1 - t0);
    matriz_liberar(&m);
    return EXIT_SUCCESS;
}
