/*
 * matriz.h - Estruturas e rotinas compartilhadas (entrada e medicao de tempo)
 * Sistemas Operacionais - PUCRS - Contagem de objetos em matriz binaria
 * Padrao: ANSI C (C89/C90)
 */
#ifndef MATRIZ_H
#define MATRIZ_H

/* Matriz binaria armazenada em vetor linear (linha-major). */
typedef struct {
    int linhas;
    int colunas;
    unsigned char *dados;   /* 0 = fundo, 1 = primeiro plano */
} Matriz;

/* Le arquivo texto: "LINHAS COLUNAS" seguido de LINHAS*COLUNAS valores 0/1.
 * Retorna 0 em sucesso e -1 em erro (mensagem ja impressa em stderr). */
int matriz_carregar(const char *caminho, Matriz *m);

/* Libera a memoria da matriz. */
void matriz_liberar(Matriz *m);

/* Relogio de parede em milissegundos (gettimeofday). */
double tempo_ms(void);

/* Desloca de vizinhanca-8 (linha, coluna). */
extern const int VIZ_DL[8];
extern const int VIZ_DC[8];

#endif
