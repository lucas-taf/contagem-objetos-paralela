/*
 * conta-objetos-paralelo.c
 * Versao PARALELA com POSIX Threads (Pthreads).
 *
 * Estrategia:
 *   1. A matriz e dividida em uma grade de BL x BC blocos (tarefas).
 *   2. FASE 1 (paralela): T threads retiram blocos de uma FILA DINAMICA
 *      (indice compartilhado protegido por mutex) e rotulam os componentes
 *      LOCAIS de cada bloco com flood fill iterativo restrito ao bloco.
 *      Rotulo local = indice linear da celula-semente + 1 (unico no programa
 *      inteiro, sem necessidade de coordenacao).
 *   3. FASE 2 (paralela): as threads retiram novamente os blocos da fila e
 *      examinam a borda INFERIOR e a borda DIREITA de cada bloco, comparando
 *      cada celula com seus 3 vizinhos do outro lado (vertical + 2 diagonais).
 *      Rotulos equivalentes sao unidos numa estrutura Union-Find compartilhada,
 *      protegida por mutex (regiao critica).
 *   4. Resultado global = soma das contagens locais - numero de unioes efetivas.
 *
 * Uso: ./conta-objetos-paralelo <arquivo> <threads> [blocos_linhas blocos_colunas] [-v]
 * Padrao: ANSI C (C89/C90) + Pthreads
 */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "matriz.h"

#define MAX_THREADS 256

/* Regiao retangular [r0,r1) x [c0,c1) processada como uma tarefa. */
typedef struct {
    int r0, r1, c0, c1;
    long locais;            /* componentes encontrados dentro do bloco */
} Bloco;

/* Estado compartilhado entre as threads. */
typedef struct {
    const Matriz *m;
    int *rotulo;            /* rotulo por celula (0 = fundo) - escrito so na fase 1 */
    int *pai;               /* Union-Find: pai[rotulo] - protegido por mtx_uf */
    Bloco *blocos;
    int nblocos;
    int proximo;            /* fila dinamica: proximo bloco - protegido por mtx_fila */
    pthread_mutex_t mtx_fila;
    pthread_mutex_t mtx_uf;
    long unioes;            /* unioes efetivas - protegido por mtx_uf */
    int erro;               /* falha de memoria em alguma thread - protegido por mtx_fila */
} Contexto;

/* Argumento individual de cada thread. */
typedef struct {
    Contexto *ctx;
    int id;
    long blocos_processados;
} ArgThread;

/* ---------- Fila dinamica de blocos ---------- */
static int proximo_bloco(Contexto *ctx)
{
    int b;
    pthread_mutex_lock(&ctx->mtx_fila);
    if (ctx->erro || ctx->proximo >= ctx->nblocos) {
        b = -1;
    } else {
        b = ctx->proximo++;
    }
    pthread_mutex_unlock(&ctx->mtx_fila);
    return b;
}

static void sinalizar_erro(Contexto *ctx)
{
    pthread_mutex_lock(&ctx->mtx_fila);
    ctx->erro = 1;
    pthread_mutex_unlock(&ctx->mtx_fila);
}

/* ---------- Union-Find (chamar SOMENTE com mtx_uf travado) ---------- */
static int uf_encontrar(int *pai, int x)
{
    while (pai[x] != x) {
        pai[x] = pai[pai[x]];   /* compressao de caminho (halving) */
        x = pai[x];
    }
    return x;
}

/* Une dois rotulos; o menor rotulo vira representante (determinismo). */
static void unir(Contexto *ctx, int a, int b)
{
    int ra, rb;
    pthread_mutex_lock(&ctx->mtx_uf);       /* inicio da regiao critica */
    ra = uf_encontrar(ctx->pai, a);
    rb = uf_encontrar(ctx->pai, b);
    if (ra != rb) {
        if (ra < rb) {
            ctx->pai[rb] = ra;
        } else {
            ctx->pai[ra] = rb;
        }
        ctx->unioes++;
    }
    pthread_mutex_unlock(&ctx->mtx_uf);     /* fim da regiao critica */
}

/* ---------- FASE 1: rotulagem local de um bloco ---------- */
static int rotular_bloco(Contexto *ctx, Bloco *bl, int **pilha, long *cap)
{
    const Matriz *m = ctx->m;
    int C = m->colunas;
    int l, c, k;
    long topo;

    bl->locais = 0;
    for (l = bl->r0; l < bl->r1; l++) {
        for (c = bl->c0; c < bl->c1; c++) {
            int semente = l * C + c;
            int rot;
            if (m->dados[semente] == 0 || ctx->rotulo[semente] != 0) {
                continue;
            }
            rot = semente + 1;                 /* rotulo globalmente unico */
            ctx->pai[rot] = rot;               /* indice exclusivo desta thread */
            bl->locais++;
            ctx->rotulo[semente] = rot;
            topo = 0;
            (*pilha)[topo++] = semente;
            while (topo > 0) {
                int cel = (*pilha)[--topo];
                int cl = cel / C, cc = cel % C;
                for (k = 0; k < 8; k++) {
                    int nl = cl + VIZ_DL[k];
                    int nc = cc + VIZ_DC[k];
                    int viz;
                    /* flood fill restrito aos limites do BLOCO */
                    if (nl < bl->r0 || nl >= bl->r1 || nc < bl->c0 || nc >= bl->c1) {
                        continue;
                    }
                    viz = nl * C + nc;
                    if (m->dados[viz] && ctx->rotulo[viz] == 0) {
                        ctx->rotulo[viz] = rot;
                        if (topo == *cap) {
                            long nova = *cap * 2;
                            int *tmp = (int *) realloc(*pilha, (size_t) nova * sizeof(int));
                            if (tmp == NULL) {
                                return -1;
                            }
                            *pilha = tmp;
                            *cap = nova;
                        }
                        (*pilha)[topo++] = viz;
                    }
                }
            }
        }
    }
    return 0;
}

static void *trabalhador_fase1(void *arg)
{
    ArgThread *a = (ArgThread *) arg;
    Contexto *ctx = a->ctx;
    long cap = 1024;
    int *pilha = (int *) malloc((size_t) cap * sizeof(int));
    int b;

    if (pilha == NULL) {
        sinalizar_erro(ctx);
        return NULL;
    }
    while ((b = proximo_bloco(ctx)) >= 0) {
        if (rotular_bloco(ctx, &ctx->blocos[b], &pilha, &cap) != 0) {
            sinalizar_erro(ctx);
            break;
        }
        a->blocos_processados++;
    }
    free(pilha);
    return NULL;
}

/* ---------- FASE 2: verificacao de fronteiras e consolidacao ---------- */
static void verificar_fronteiras(Contexto *ctx, const Bloco *bl)
{
    const Matriz *m = ctx->m;
    int L = m->linhas, C = m->colunas;
    int l, c, d;

    /* Fronteira HORIZONTAL (borda inferior): linha r1-1 x linha r1.
     * Compara com (r1, c-1), (r1, c), (r1, c+1): vertical + 2 diagonais.
     * As diagonais que cruzam a coluna de corte tratam o encontro de 4 blocos. */
    if (bl->r1 < L) {
        l = bl->r1 - 1;
        for (c = bl->c0; c < bl->c1; c++) {
            int a = ctx->rotulo[l * C + c];
            if (a == 0) {
                continue;
            }
            for (d = -1; d <= 1; d++) {
                int nc = c + d;
                int b;
                if (nc < 0 || nc >= C) {
                    continue;
                }
                b = ctx->rotulo[(l + 1) * C + nc];
                if (b != 0 && b != a) {
                    unir(ctx, a, b);
                }
            }
        }
    }
    /* Fronteira VERTICAL (borda direita): coluna c1-1 x coluna c1.
     * Compara com (l-1, c1), (l, c1), (l+1, c1): horizontal + 2 diagonais. */
    if (bl->c1 < C) {
        c = bl->c1 - 1;
        for (l = bl->r0; l < bl->r1; l++) {
            int a = ctx->rotulo[l * C + c];
            if (a == 0) {
                continue;
            }
            for (d = -1; d <= 1; d++) {
                int nl = l + d;
                int b;
                if (nl < 0 || nl >= L) {
                    continue;
                }
                b = ctx->rotulo[nl * C + (c + 1)];
                if (b != 0 && b != a) {
                    unir(ctx, a, b);
                }
            }
        }
    }
}

static void *trabalhador_fase2(void *arg)
{
    ArgThread *a = (ArgThread *) arg;
    Contexto *ctx = a->ctx;
    int b;
    while ((b = proximo_bloco(ctx)) >= 0) {
        verificar_fronteiras(ctx, &ctx->blocos[b]);
    }
    return NULL;
}

/* Cria T threads executando 'rotina' e aguarda todas (pthread_join).
 * O join funciona como barreira entre as fases. Retorna 0 em sucesso. */
static int executar_fase(void *(*rotina)(void *), ArgThread *args, int T)
{
    pthread_t th[MAX_THREADS];
    int i, rc, criadas = 0, falhou = 0;

    for (i = 0; i < T; i++) {
        rc = pthread_create(&th[i], NULL, rotina, &args[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            falhou = 1;
            break;
        }
        criadas++;
    }
    for (i = 0; i < criadas; i++) {
        rc = pthread_join(th[i], NULL);
        if (rc != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(rc));
            falhou = 1;
        }
    }
    return falhou ? -1 : 0;
}

static void uso(const char *prog)
{
    fprintf(stderr,
            "Uso: %s <arquivo> <threads> [blocos_linhas blocos_colunas] [-v]\n"
            "  threads        : 1..%d unidades de execucao\n"
            "  blocos_linhas  : divisoes na vertical (padrao = threads)\n"
            "  blocos_colunas : divisoes na horizontal (padrao = 1)\n"
            "  -v             : mostra contagens locais por bloco e unioes\n",
            prog, MAX_THREADS);
}

int main(int argc, char *argv[])
{
    Matriz m;
    Contexto ctx;
    ArgThread args[MAX_THREADS];
    int T, BL, BC, i, j, b, verbose = 0, nargs = 0, rc;
    char *pos[4];
    long soma_locais = 0, objetos;
    double t0, t1;
    long n;

    /* separa a opcao -v dos argumentos posicionais */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (nargs < 4) {
            pos[nargs++] = argv[i];
        } else {
            uso(argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (nargs != 2 && nargs != 4) {
        uso(argv[0]);
        return EXIT_FAILURE;
    }
    T = atoi(pos[1]);
    if (T < 1 || T > MAX_THREADS) {
        uso(argv[0]);
        return EXIT_FAILURE;
    }
    BL = T;
    BC = 1;
    if (nargs == 4) {
        BL = atoi(pos[2]);
        BC = atoi(pos[3]);
        if (BL < 1 || BC < 1) {
            uso(argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (matriz_carregar(pos[0], &m) != 0) {
        return EXIT_FAILURE;
    }
    if (BL > m.linhas) BL = m.linhas;     /* nao ha blocos vazios */
    if (BC > m.colunas) BC = m.colunas;

    n = (long) m.linhas * m.colunas;
    memset(&ctx, 0, sizeof(ctx));
    ctx.m = &m;
    ctx.nblocos = BL * BC;
    ctx.rotulo = (int *) calloc((size_t) n, sizeof(int));
    ctx.pai = (int *) malloc((size_t) (n + 1) * sizeof(int));
    ctx.blocos = (Bloco *) malloc((size_t) ctx.nblocos * sizeof(Bloco));
    if (ctx.rotulo == NULL || ctx.pai == NULL || ctx.blocos == NULL) {
        fprintf(stderr, "Erro: memoria insuficiente\n");
        free(ctx.rotulo); free(ctx.pai); free(ctx.blocos);
        matriz_liberar(&m);
        return EXIT_FAILURE;
    }

    /* Particionamento: as sobras da divisao sao distribuidas uma a uma
     * para os primeiros blocos (diferenca maxima de 1 linha/coluna). */
    b = 0;
    for (i = 0; i < BL; i++) {
        for (j = 0; j < BC; j++) {
            Bloco *bl = &ctx.blocos[b++];
            bl->r0 = (int) ((long) i * m.linhas / BL);
            bl->r1 = (int) ((long) (i + 1) * m.linhas / BL);
            bl->c0 = (int) ((long) j * m.colunas / BC);
            bl->c1 = (int) ((long) (j + 1) * m.colunas / BC);
            bl->locais = 0;
        }
    }

    rc = pthread_mutex_init(&ctx.mtx_fila, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_mutex_init: %s\n", strerror(rc));
        free(ctx.rotulo); free(ctx.pai); free(ctx.blocos);
        matriz_liberar(&m);
        return EXIT_FAILURE;
    }
    rc = pthread_mutex_init(&ctx.mtx_uf, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_mutex_init: %s\n", strerror(rc));
        pthread_mutex_destroy(&ctx.mtx_fila);
        free(ctx.rotulo); free(ctx.pai); free(ctx.blocos);
        matriz_liberar(&m);
        return EXIT_FAILURE;
    }
    for (i = 0; i < T; i++) {
        args[i].ctx = &ctx;
        args[i].id = i;
        args[i].blocos_processados = 0;
    }

    t0 = tempo_ms();
    /* FASE 1 - rotulagem local (paralela) */
    ctx.proximo = 0;
    rc = executar_fase(trabalhador_fase1, args, T);
    /* FASE 2 - fronteiras/consolidacao (paralela) */
    if (rc == 0 && !ctx.erro) {
        ctx.proximo = 0;    /* seguro: nenhuma thread ativa apos o join */
        rc = executar_fase(trabalhador_fase2, args, T);
    }
    /* Contagem final (sequencial, O(numero de blocos)) */
    for (b = 0; b < ctx.nblocos; b++) {
        soma_locais += ctx.blocos[b].locais;
    }
    objetos = soma_locais - ctx.unioes;
    t1 = tempo_ms();

    if (rc != 0 || ctx.erro) {
        fprintf(stderr, "Erro durante a execucao paralela\n");
    } else {
        printf("Versao: paralela (pthreads)\n");
        printf("Dimensoes: %d x %d\n", m.linhas, m.colunas);
        printf("Threads: %d | Blocos: %d x %d (%d tarefas)\n", T, BL, BC, ctx.nblocos);
        if (verbose) {
            for (b = 0; b < ctx.nblocos; b++) {
                const Bloco *bl = &ctx.blocos[b];
                printf("  Bloco %2d linhas[%d,%d) colunas[%d,%d): %ld objeto(s) local(is)\n",
                       b, bl->r0, bl->r1, bl->c0, bl->c1, bl->locais);
            }
            for (i = 0; i < T; i++) {
                printf("  Thread %d processou %ld bloco(s) na fase 1\n",
                       i, args[i].blocos_processados);
            }
            printf("  Soma local: %ld | Unioes nas fronteiras: %ld\n",
                   soma_locais, ctx.unioes);
        }
        printf("Objetos: %ld\n", objetos);
        printf("Tempo_ms: %.3f\n", t1 - t0);
    }

    /* Liberacao de recursos */
    pthread_mutex_destroy(&ctx.mtx_fila);
    pthread_mutex_destroy(&ctx.mtx_uf);
    free(ctx.rotulo);
    free(ctx.pai);
    free(ctx.blocos);
    matriz_liberar(&m);
    return (rc != 0 || ctx.erro) ? EXIT_FAILURE : EXIT_SUCCESS;
}
