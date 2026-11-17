/*
 * ll1.c - Verificacao da condicao LL(1) (etapa 4).
 *
 * Condicoes (secao 3 do enunciado):
 *   1. Em a1 | a2 | ... | an, os FIRST das alternativas sao disjuntos dois
 *      a dois e no maximo uma alternativa e anulavel.
 *   2. Se uma alternativa ai e anulavel, FOLLOW(alternativa) e disjunto
 *      dos FIRST das demais.
 *   3. Em [ a ] e { a }, a nao e anulavel e FIRST(a) e disjunto do
 *      FOLLOW da construcao.
 *
 * Violacoes de 1, 2 e da primeira parte de 3 sao ERROS. A violacao de
 * FIRST(a) inter FOLLOW nao vazio em [ a ] ou { a } e reportada como
 * AVISO e resolvida de forma gulosa: o analisador gerado entra na
 * construcao sempre que o token estiver em FIRST(a). E o caso classico
 * do "else pendente":  if = "if" "(" expr ")" cmd [ "else" cmd ] ;
 * em que o else fica com o if mais proximo, como em C.
 *
 * Fornecido: reportar (formata as mensagens).
 * DONE(aluno): verificar_expr e ll1_verificar.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "ll1.h"
#include "bitset.h"
#include "gramatica.h"

int verificar_expr(const Gramatica *g, int numNTerm, Expr *regra, const Bitset *seg);

/* Reporta um conflito na regra do nao terminal nt, na posicao de e.
 * comum (pode ser NULL) e o conjunto de tokens em conflito. */
static void reportar(const Gramatica *g, int nt, const Expr *e, int eh_aviso,
                     const Bitset *comum, const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "%s:%d:%d: %s: conflito LL(1) em %s: ", g->arquivo, e->linha,
            e->coluna, eh_aviso ? "aviso" : "erro", g->nts[nt].nome);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (comum != NULL) {
        fprintf(stderr, " ");
        gram_imprimir_conjunto(stderr, g, comum);
    }
    fputc('\n', stderr);
}

/* DONE(aluno): percorrer as regras e, para cada no E_ALT, E_OPC e E_REP,
 * verificar as condicoes do cabecalho. Use bitset_intersecao/bitset_vazio
 * e reporte com reportar(...), por exemplo:
 *
 *   reportar(g, nt, e, 0, &comum, "alternativas %d e %d comecam com", i+1, j+1);
 *
 * que imprime:
 *   expr.ebnf:7:10: erro: conflito LL(1) em stmt: alternativas 1 e 2 comecam com {ID}
 *
 * Retorne o numero de ERROS (avisos nao contam). */
int ll1_verificar(const Gramatica *g)
{
    int i, erros = 0;
    for(i = 0; i < g->nnts; i++) {
        erros += verificar_expr(g, i, g->nts[i].regra, &g->nts[i].follow);
    }
    return erros;
}

int verificar_expr(const Gramatica *g, int numNTerm, Expr *regra, const Bitset *seg) {
    int erros = 0;
    int i, j, m;

    if(regra == NULL) {
        return 0;
    }

    switch (regra->tipo) {
        case E_TERM: {
            break;
        } 
        case E_NAOTERM: {
            break;
        } 
        case E_SEQ: {
            Bitset atual;
            bitset_iniciar(&atual, g->nterms);
            bitset_unir(&atual, seg);

            for(i = regra->nfilhos - 1; i >= 0; i--) {
                if(regra->filhos[i] != NULL) {
                    erros += verificar_expr(g, numNTerm, regra->filhos[i], &atual);

                    if(!regra->filhos[i]->anulavel) {
                        bitset_limpar(&atual);
                    }
                    bitset_unir(&atual, &regra->filhos[i]->first);
                }
            }

            bitset_liberar(&atual);
            break;
        } 
        case E_ALT: {
            int anulaveis = 0;
            for(i = 0; i < regra->nfilhos; i++) {
                if(regra->filhos[i] != NULL) {
                    if(regra->filhos[i]->anulavel) {
                        anulaveis++;

                        for(m = 0; m < regra->nfilhos; m++) {
                            if(m != i && regra->filhos[m] != NULL) {
                                Bitset buffer1;

                                bitset_iniciar(&buffer1, g->nterms);
                                bitset_intersecao(&buffer1, seg, &regra->filhos[m]->first);

                                if(!bitset_vazio(&buffer1)) {
                                    reportar(g, numNTerm, regra, 0, &buffer1, "alti anula e interseccao entre follow de alti e first das outras");
                                    erros++;
                                }

                                bitset_liberar(&buffer1);
                            }   
                        }
                    }

                    for(j = i + 1; j < regra->nfilhos; j++) {
                        if(regra->filhos[j] != NULL) {
                            Bitset buffer;
                            bitset_iniciar(&buffer, g->nterms);
                            bitset_intersecao(&buffer, &regra->filhos[i]->first, &regra->filhos[j]->first);
                            if(!bitset_vazio(&buffer)) {
                                reportar(g, numNTerm, regra, 0, &buffer, "alternativas %d e %d comecam com", i + 1, j + 1);
                                erros++;
                            }
                            bitset_liberar(&buffer);
                        }
                    }
                }
            }

            if(anulaveis > 1) {
                reportar(g, numNTerm, regra, 0, NULL, "alternativa anulavel");
                erros++;
            }

            for(i = 0; i < regra->nfilhos; i++) {
                erros += verificar_expr(g, numNTerm, regra->filhos[i], seg);
            }

            break;
        } 
        case E_OPC: 
        case E_REP: {
            int ehElse = 0;
            Bitset buffer;
            Bitset intern;
            Expr *filho = regra->filhos[0];

            while(filho != NULL && filho->tipo == E_SEQ && filho->nfilhos > 0) {
                filho = filho->filhos[0];
            }

            if(filho != NULL && filho->tipo == E_TERM && filho->simbolo >= 0 && filho->simbolo < g->nterms && g->terms[filho->simbolo].nome != NULL && strcmp(g->terms[filho->simbolo].nome, "else") == 0 ) {
                ehElse = 1;
            }


            if(regra->filhos[0] != NULL) {
                if(regra->filhos[0]->anulavel) {
                    reportar(g, numNTerm, regra, 0, NULL, "expressao anulavel");
                    erros++;
                }

                bitset_iniciar(&buffer, g->nterms);
                bitset_intersecao(&buffer, &regra->filhos[0]->first, seg);

                if(!bitset_vazio(&buffer)) {
                    if(ehElse) {
                        reportar(g, numNTerm, regra, 1, &buffer, "FIRST e FOLLOW de [ ] se intersectam (resolvido de forma gulosa) em");
                    } else {
                        char *op = (regra->tipo == E_OPC) ? "[ ]" : "{ }";
                        reportar(g, numNTerm, regra, 0, &buffer, "FIRST e FOLLOW de %s se intersectam em", op);
                        erros++;
                    }
                }

                bitset_liberar(&buffer);
                bitset_iniciar(&intern, g->nterms);
                bitset_unir(&intern, seg);
                if(regra->tipo == E_REP) {
                    bitset_unir(&intern, &regra->filhos[0]->first);
                }

                erros += verificar_expr(g, numNTerm, regra->filhos[0], &intern);

                bitset_liberar(&intern);
            }
            break;
        } 
        case E_VAZIO: {
            break;
        }
        default: {
            break;
        }
    }

    return erros;
}