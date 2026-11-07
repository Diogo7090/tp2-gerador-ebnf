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
 * TODO(aluno): verificar_expr e ll1_verificar.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include "ll1.h"
#include "bitset.h"
#include "gramatica.h"
#include "util.h"

typedef struct noRegra noRegra;
void addRegraNaTabela(noRegra ***tabelaPreditiva, Expr* regra, int posicaoNTerm, int posicaoTerm);
void liberarCelulas(noRegra *celula);
void liberarRecursos(const Gramatica *g, noRegra*** tabelaPreditiva);
int preencherTabelaPreditiva2B(const Gramatica *g, Expr* regra, noRegra*** tabelaPreditiva, int numNTerm);
int verificar_expr(const Gramatica *g, int numNTerm, Expr *regra );

struct noRegra{
    struct noRegra *proximo;
    Expr *regra;
};
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

/* TODO(aluno): percorrer as regras e, para cada no E_ALT, E_OPC e E_REP,
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

    /*
        chamar para cada terminar A->a
        1 -  Para cada a em primeiro A, colocar referencia producao M[A, a]
        2 -verificar se first naoTerm A nao possui vazio
        2 -se vazio existir adicionar a referencia a prod posicao M[A, b] e  cada b follow de A
        2 - se vazio exisit em first de a e seguinte em A colococar regra em M[A, $]
        3 - posições vazia marcar como erro
    */
    int erros = 0;
    int i, j, p;
    NaoTerminal nTermAtual;
    noRegra *celulaAtual;


    noRegra ***tabelaPreditiva = (noRegra***) calloc(g->nnts, sizeof(noRegra**));
    for(i = 0; i < g->nnts; i++) {
        tabelaPreditiva[i] = (noRegra**) calloc(g->nterms, sizeof(noRegra*));
        if(tabelaPreditiva[i] == NULL) {
            printf("Erro alocar alocar coluna\n");
        }
    }

    for(i = 0; i < g->nnts; i++) {
        nTermAtual = g->nts[i];
        if(!bitset_vazio(&(nTermAtual.first))) {

            for(j = bitset_proximo(&nTermAtual.first, 0); j >= 0; j = bitset_proximo(&nTermAtual.first, j+1)) {
                if(bitset_tem(&g->nts[i].first, j)) {
                    addRegraNaTabela(tabelaPreditiva, g->nts[i].regra, i, j);
                }
            }
        }

        if(nTermAtual.anulavel) {
            for(p = bitset_proximo(&nTermAtual.follow, 0); p >= 0; p = bitset_proximo(&nTermAtual.follow, p+1)) {
                if(bitset_tem(&nTermAtual.follow, p)) {
                    addRegraNaTabela(tabelaPreditiva, nTermAtual.regra, i, p);
                }
            }

            /*
                verificar parte b da regra 2
            */
            (void)preencherTabelaPreditiva2B(g, nTermAtual.regra, tabelaPreditiva, i);
        }
    }

    for(i = 0; i < g->nnts; i++) {
        for(j = 0; j < g->nterms; j++) {
            celulaAtual = tabelaPreditiva[i][j];
            if(celulaAtual != NULL && celulaAtual->proximo != NULL) {
                reportar(g, i, celulaAtual->regra, 0, NULL, "conflito de multiplas regras para o terminal %d", j);
                erros++;
            }
        }
    }

    for(i = 0; i < g->nnts; i++) {
        erros += verificar_expr(g, i, g->nts[i].regra);
    }
    liberarRecursos(g, tabelaPreditiva);
    tabelaPreditiva = NULL;
    (void) reportar;
    (void) g;
    return erros;
}

int preencherTabelaPreditiva2B(const Gramatica *g, Expr* regra, noRegra*** tabelaPreditiva, int numNTerm) {
    int i, quantAnul;
    if(regra == NULL) {
        return 0;
    }

    switch(regra->tipo) {
        case E_TERM: {
            break;
        } 
        case E_NAOTERM: {
            if(regra->anulavel) {
                return 1;
            }

            break;
        } 
        case E_ALT: {
            for(i = 0; i < regra->nfilhos; i++) {
                if(regra->filhos[i] != NULL && regra->filhos[i]->anulavel) {

                    if(preencherTabelaPreditiva2B(g, regra->filhos[i], tabelaPreditiva, numNTerm)) {
                        addRegraNaTabela(tabelaPreditiva, regra->filhos[i], numNTerm, 0);
                    }
                }
            }
            break;
        } 
        case E_SEQ: {
            quantAnul = 0;
            for(i = 0; i  < regra->nfilhos; i++) {
                if(regra->filhos[i] != NULL) {
                    quantAnul += preencherTabelaPreditiva2B(g, regra->filhos[i], tabelaPreditiva, numNTerm);
                }
            }

            if(quantAnul == regra->nfilhos) {
                return 1;
            }

            return 0;
            break;
        } 
        case E_OPC: {
            return 1;
            break;
        } 
        case E_REP: {
            return 1;
            break;
        } 
        case E_VAZIO: {
            return 1;
            break;
        }

    }
    return 0;
}


void addRegraNaTabela(noRegra ***tabelaPreditiva, Expr* regra, int posicaoNTerm, int posicaoTerm) {
    noRegra *noTemp = (noRegra*) malloc(sizeof(noRegra));
    noTemp->regra = regra;
    noTemp->proximo = tabelaPreditiva[posicaoNTerm][posicaoTerm];

    tabelaPreditiva[posicaoNTerm][posicaoTerm] = noTemp;
}

void liberarRecursos(const Gramatica *g, noRegra*** tabelaPreditiva) {
    int i, j;
    for(i = 0; i < g->nnts; i++) {
        for(j = 0; j < g->nterms; j++) {
            liberarCelulas(tabelaPreditiva[i][j]);
        }
    }
}

void liberarCelulas(noRegra *celula) {
    noRegra *noAux = celula;

    while(celula != NULL) {
        celula = celula->proximo;
        free(noAux);
        noAux = celula;
    }
}

int verificar_expr(const Gramatica *g, int numNTerm, Expr *regra ) {
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
            for(i = 0; i < regra->nfilhos; i++) {
                erros += verificar_expr(g, numNTerm, regra->filhos[i]);
            }
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
                                bitset_intersecao(&buffer1, &g->nts[numNTerm].follow, &regra->filhos[m]->first);

                                if(!bitset_vazio(&buffer1)) {
                                    reportar(g, numNTerm, regra, 0, &buffer1, "alti  anula e interseccao entre follow de alti e first das outras");
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
                erros += verificar_expr(g, numNTerm, regra->filhos[i]);
            }

            break;
        } 
        case E_OPC: 
        case E_REP: {
            int i;
            int ehElse = 0;
            Bitset buffer;

            for(i = 0; i < regra->nfilhos; i++) {
                if(regra->filhos[i] != NULL && regra->filhos[i]->simbolo >= 0 && regra->filhos[i]->simbolo < g->nterms) {
                    if(g->terms[regra->filhos[i]->simbolo].nome != NULL) {
                        if(strcmp(g->terms[regra->filhos[i]->simbolo].nome, "else") == 0) {
                            ehElse = 1;
                        } 
                    }
                }
            }

            if(regra->filhos[0] != NULL) {
                if(regra->filhos[0]->anulavel) {
                    reportar(g, numNTerm, regra, 0, NULL, "expressao anulavel");
                    erros++;
                }

                bitset_iniciar(&buffer, g->nterms);
                bitset_intersecao(&buffer, &regra->filhos[0]->first, &g->nts[numNTerm].follow);

                if(!bitset_vazio(&buffer)) {
                    if(ehElse) {
                        reportar(g, numNTerm, regra, 1, &buffer, "FIRST e FOLLOW de [ ] se intersectam (resolvido de forma gulosa) em");
                    }
                }

                bitset_liberar(&buffer);

                erros += verificar_expr(g, numNTerm, regra->filhos[0]);
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