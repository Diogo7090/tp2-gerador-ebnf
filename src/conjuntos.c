/*
 * conjuntos.c - Calculo de anulavel, FIRST e FOLLOW (etapa 3).
 *
 * Fornecido: conj_iniciar e conj_imprimir.
 *
 * TODO(aluno): conj_anulavel, conj_first e conj_follow. Os tres sao
 * calculados por iteracao ate ponto fixo: repita uma passada sobre todas
 * as regras enquanto algum conjunto (ou flag) mudar. bitset_unir devolve
 * 1 quando o conjunto de destino muda, o que ajuda a detectar o ponto fixo.
 */
#include <stdio.h>
#include <string.h>
#include "conjuntos.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* Fornecido                                                          */
/* ------------------------------------------------------------------ */

static void iniciar_expr(Expr *e, void *ctx)
{
    int n = *(int *) ctx;
    bitset_liberar(&e->first);
    bitset_liberar(&e->follow);
    bitset_iniciar(&e->first, n);
    bitset_iniciar(&e->follow, n);
    e->anulavel = 0;
}

void conj_iniciar(Gramatica *g)
{
    int i, n = g->nterms;
    for (i = 0; i < g->nnts; i++) {
        bitset_liberar(&g->nts[i].first);
        bitset_liberar(&g->nts[i].follow);
        bitset_iniciar(&g->nts[i].first, n);
        bitset_iniciar(&g->nts[i].follow, n);
        g->nts[i].anulavel = 0;
        expr_percorrer(g->nts[i].regra, iniciar_expr, &n);
    }
}

void conj_imprimir(FILE *f, const Gramatica *g)
{
    int i;
    size_t largura = strlen("NAO TERMINAL");
    for (i = 0; i < g->nnts; i++)
        if (strlen(g->nts[i].nome) > largura)
            largura = strlen(g->nts[i].nome);
    fprintf(f, "%-*s  ANULAVEL  FIRST / FOLLOW\n", (int) largura, "NAO TERMINAL");
    for (i = 0; i < g->nnts; i++) {
        fprintf(f, "%-*s  %-8s  FIRST  = ", (int) largura, g->nts[i].nome,
                g->nts[i].anulavel ? "sim" : "nao");
        gram_imprimir_conjunto(f, g, &g->nts[i].first);
        fprintf(f, "\n%-*s            FOLLOW = ", (int) largura, "");
        gram_imprimir_conjunto(f, g, &g->nts[i].follow);
        fputc('\n', f);
    }
}

/* ------------------------------------------------------------------ */
/* A implementar                                                      */
/* ------------------------------------------------------------------ */

/* TODO(aluno): calcular e->anulavel para cada no Expr e g->nts[i].anulavel.
 *   E_TERM: 0        E_VAZIO: 1        E_NAOTERM: anulavel do nao terminal
 *   E_SEQ: 1 se todos os filhos forem anulaveis
 *   E_ALT: 1 se algum filho for anulavel
 *   E_OPC, E_REP: 1 (calcule tambem o do filho)
 * Repita ate nada mudar. */

/* g->nts[i].anulavel ----- lista de todos os não terminais anuláveis */
/* e->simbolo ------ pega o id atual da expr */
/* e->nfilhos -------- definido em gramaticas.c*/
static int calc_anulavel (Gramatica *g, Expr *e){
    int a;
    int i;

    if (e->tipo == E_TERM){
        e->anulavel = 0;
        return 0;
    }
    else if (e->tipo == E_VAZIO){
        e->anulavel = 1;
        return 1;
    }
    else if (e->tipo == E_NAOTERM){
        e->anulavel = g->nts[e->simbolo].anulavel;
        return e->anulavel;
    }
    else if (e->tipo == E_OPC || e->tipo == E_REP) {
        calc_anulavel(g, e->filhos[0]);
        e->anulavel = 1;
        return 1;
    }
    else if (e->tipo == E_SEQ){
        /* precisa verificar se todos os filhos são anuláveis */
        a = 1;
        i = 0;
        while (i < e->nfilhos){
            if(calc_anulavel(g, e->filhos[i]) == 0){
                a=0;
            }
            i++;
        }
        e->anulavel = a;
        return e->anulavel;
    }
    else if (e->tipo == E_ALT){
        /* precisa verificar se apenas 1 dos filhos é anulavel */
        a = 0;
        i = 0;
        while (i < e->nfilhos){
            if(calc_anulavel(g, e->filhos[i]) == 1){
                a = 1;
            }
            i++;
        }
        e->anulavel = a;
        return e->anulavel;
    }
    return 0;
}

void conj_anulavel(Gramatica *g)
{
    int mudou = 1;
    int i;

    i=0;
    while (i<g->nnts){
        g->nts[i].anulavel = 0;
        i++;
    }

    while (mudou) {
        mudou = 0;
        i=0;
        while (i<g->nnts){
            if (g->nts[i].regra != NULL) {
                int novo_valor_anulavel = calc_anulavel(g, g->nts[i].regra);
                if (novo_valor_anulavel != g->nts[i].anulavel) {
                    g->nts[i].anulavel = novo_valor_anulavel;
                    mudou = 1;
                }
            }
            i++;
        }
    }
}

/* TODO(aluno): calcular e->first para cada no Expr e g->nts[i].first.
 *   E_TERM t: {t}     E_NAOTERM B: FIRST(B)     E_VAZIO: {}
 *   E_SEQ a1..an: FIRST(a1) U FIRST(a2) ... ate o primeiro ai nao anulavel
 *   E_ALT, E_OPC, E_REP: uniao dos FIRST dos filhos
 * Calcule o FIRST de TODOS os filhos de uma sequencia, mesmo os que
 * nao contribuem para o FIRST dela: o gerador de codigo usa-os. */
void conj_first(Gramatica *g)
{
    (void) g;
}

/* TODO(aluno): calcular e->follow (o que pode vir logo apos cada
 * construcao) e g->nts[i].follow. FOLLOW(inicial) contem $ (TERM_FIM).
 * Para cada regra A = alfa, faca follow(alfa) U= FOLLOW(A) e propague:
 *   E_SEQ a1..an:  follow(ai) U= FIRST(ai+1 ... an), mais follow(seq)
 *                  se ai+1 ... an for anulavel
 *   E_ALT, E_OPC:  follow(filho) U= follow(no)
 *   E_REP { a }:   follow(a) U= follow(no) U FIRST(a)
 *   E_NAOTERM B:   FOLLOW(B) U= follow(no)
 * Repita ate nada mudar. */
void conj_follow(Gramatica *g)
{
    (void) g;
}
