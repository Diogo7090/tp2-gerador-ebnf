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


/* aparentemente, feito */
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
        /* precisa verificar se há no mínimo 1 filho que é anulavel */
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


/* aparentemente, feito */
/* TODO(aluno): calcular e->first para cada no Expr e g->nts[i].first.
 *   E_TERM t: {t}     E_NAOTERM B: FIRST(B)     E_VAZIO: {}
 *   E_SEQ a1..an: FIRST(a1) U FIRST(a2) ... ate o primeiro ai nao anulavel
 *   E_ALT, E_OPC, E_REP: uniao dos FIRST dos filhos
 * Calcule o FIRST de TODOS os filhos de uma sequencia, mesmo os que
 * nao contribuem para o FIRST dela: o gerador de codigo usa-os. */
static void calc_first(Gramatica *g, Expr *e, Bitset *resultado) {
    if (e == NULL) {
        return;
    }

    if (e->first.w == NULL) {
        bitset_iniciar(&e->first, g->nterms);
        bitset_limpar(&e->first);
    }

    if (e->tipo == E_VAZIO) {
        return;
    }
    else if (e->tipo == E_NAOTERM) {
        bitset_unir(resultado, &g->nts[e->simbolo].first);
    }
    else if (e->tipo == E_OPC || e->tipo == E_REP) {
        if (e->nfilhos > 0 && e->filhos[0] != NULL) {
            calc_first(g, e->filhos[0], resultado);
        }
    }
    else if (e->tipo == E_SEQ || e->tipo == E_ALT) {
        int i = 0;
        int continua = 1;
        Bitset f_filho;

        bitset_iniciar(&f_filho, g->nterms);

        while (i < e->nfilhos) {
            bitset_limpar(&f_filho);

            calc_first(g, e->filhos[i], &f_filho);

            if (e->tipo == E_ALT || continua) {
                bitset_unir(resultado, &f_filho);
            }

            if (e->tipo == E_SEQ && !e->filhos[i]->anulavel) {
                continua = 0;
            }

            i++;
        }

        bitset_liberar(&f_filho);
    }
    else if (e->tipo == E_TERM) {
        bitset_add(resultado, e->simbolo);
    }

    bitset_unir(&e->first, resultado);
}

void conj_first(Gramatica *g) 
{
    int mudou = 1;
    int i;
    Bitset f_regra;

    i = 0;
    while (i < g->nnts) {
        if (g->nts[i].first.w == NULL) {
            bitset_iniciar(&g->nts[i].first, g->nterms);
        }
        bitset_limpar(&g->nts[i].first);
        i++;
    }

    bitset_iniciar(&f_regra, g->nterms);

    while (mudou) {
        mudou = 0;
        i = 0;
        while (i < g->nnts) {
            if (g->nts[i].regra != NULL) {
                bitset_limpar(&f_regra);
                calc_first(g, g->nts[i].regra, &f_regra);
                if (bitset_unir(&g->nts[i].first, &f_regra)) {
                    mudou = 1;
                }
            }
            i++;
        }
    }

    bitset_liberar(&f_regra);
}

/* aparentemente, feito */
/* TODO(aluno): calcular e->follow (o que pode vir logo apos cada
 * construcao) e g->nts[i].follow. FOLLOW(inicial) contem $ (TERM_FIM).
 * Para cada regra A = alfa, faca follow(alfa) U= FOLLOW(A) e propague:
 *   E_SEQ a1..an:  follow(ai) U= FIRST(ai+1 ... an), mais follow(seq)
 *                  se ai+1 ... an for anulavel
 *   E_ALT, E_OPC:  follow(filho) U= follow(no)
 *   E_REP { a }:   follow(a) U= follow(no) U FIRST(a)
 *   E_NAOTERM B:   FOLLOW(B) U= follow(no)
 * Repita ate nada mudar. */
static int calc_follow(Gramatica *g, Expr *e, Bitset *follow_no) {
    int i, k, cauda_anulavel;
    int mudou = 0;
    Bitset f;

    if (e == NULL) {
        return 0;
    }

    if (e->follow.w == NULL) {
        bitset_iniciar(&e->follow, g->nterms);
        bitset_limpar(&e->follow);
    }

    bitset_unir(&e->follow, follow_no);

    if (e->tipo == E_NAOTERM) {
        mudou |= bitset_unir(&g->nts[e->simbolo].follow, follow_no);
    }
    else if (e->tipo == E_TERM || e->tipo == E_VAZIO) {
        return mudou;
    }
    else if (e->tipo == E_ALT || e->tipo == E_OPC) {
        i = 0;

        while (i < e->nfilhos) {
            mudou |= calc_follow(g, e->filhos[i], follow_no);
            i++;
        }
    }
    else if (e->tipo == E_REP) {
        if (e->nfilhos > 0 && e->filhos[0] != NULL) {
            Bitset follow_rep;

            bitset_iniciar(&follow_rep, g->nterms);

            bitset_unir(&follow_rep, follow_no);
            bitset_unir(&follow_rep, &e->filhos[0]->first);

            mudou |= calc_follow(g, e->filhos[0], &follow_rep);

            bitset_liberar(&follow_rep);
        }
    }
    else if (e->tipo == E_SEQ) {
        bitset_iniciar(&f, g->nterms);
        i = 0;

        while (i < e->nfilhos) {
            if (e->filhos[i] != NULL) {
                bitset_limpar(&f);

                k = i + 1;
                cauda_anulavel = 1;

                while (k < e->nfilhos) {
                    if (e->filhos[k] != NULL) {
                        bitset_unir(&f, &e->filhos[k]->first);

                        if (!e->filhos[k]->anulavel) {
                            cauda_anulavel = 0;
                            break;
                        }
                    }

                    k++;
                }

                if (cauda_anulavel) {
                    bitset_unir(&f, follow_no);
                }

                mudou |= calc_follow(g, e->filhos[i], &f);
            }

            i++;
        }

        bitset_liberar(&f);
    }

    return mudou;
}

void conj_follow(Gramatica *g)
{
    int mudou = 1;
    int i;
    Bitset f_inicial;

    i = 0;
    while (i < g->nnts) {
        if (g->nts[i].follow.w == NULL) {
            bitset_iniciar(&g->nts[i].follow, g->nterms);
        }

        bitset_limpar(&g->nts[i].follow);
        i++;
    }

    if (g->nnts > 0) {
        bitset_add(&g->nts[0].follow, TERM_FIM);
    }

    bitset_iniciar(&f_inicial, g->nterms);

    while (mudou) {
        mudou = 0;
        i = 0;

        while (i < g->nnts) {
            if (g->nts[i].regra != NULL) {
                bitset_limpar(&f_inicial);

                bitset_unir(&f_inicial, &g->nts[i].follow);

                if (calc_follow(g, g->nts[i].regra, &f_inicial)) {
                    mudou = 1;
                }
            }

            i++;
        }
    }

    bitset_liberar(&f_inicial);
}
