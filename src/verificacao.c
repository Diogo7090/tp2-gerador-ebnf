/*
 * verificacao.c - Verificacoes semanticas da gramatica (etapa 2).
 *
 * Ja implementados (use como modelo):
 *   - verif_indefinidos: nao terminais usados mas nao definidos;
 *   - verif_terminais:   terminais nomeados sem declaracao.
 *
 * TODO(aluno):
 *   - verif_inalcancaveis      (aviso)
 *   - verif_improdutivos       (erro)
 *   - verif_recursao_esquerda  (erro; direta e indireta)
 *
 * A duplicidade de definicoes ja e detectada durante a leitura
 * (ebnf_parser.c).
 */
#include <stdlib.h>
#include <string.h>
#include "verificacao.h"
#include "gramatica.h"
#include "microc_mapa.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* Fornecido                                                          */
/* ------------------------------------------------------------------ */

static void analisarNoReferencia(Expr *no, void *contexto);
static void visitarNaoTerminal(int indice, Gramatica *g, int *visitados);
typedef struct {
    Gramatica *gramatica;
    int *visitados;
} contextoAnalise;


static int verif_indefinidos(Gramatica *g)
{
    int i, erros = 0;
    for (i = 0; i < g->nnts; i++) {
        if (g->nts[i].regra == NULL) {
            diag_erro(g->nts[i].linha, g->nts[i].coluna,
                      "nao terminal %s usado mas nao definido", g->nts[i].nome);
            erros++;
        }
    }
    return erros;
}

static int verif_terminais(Gramatica *g, int modo_microc)
{
    int i, erros = 0, carregado = 0;
    Terminal *t;

    if (modo_microc)
        carregado = microc_carregar_tokens_h(g->tokens_externos) == 0;

    for (i = 1; i < g->nterms; i++) {
        t = &g->terms[i];
        if (modo_microc) {
            if (t->literal && microc_nome_literal(t->nome) == NULL) {
                diag_erro(t->linha, t->coluna, "literal %s nao corresponde a nenhum "
                          "token de Micro C", t->exibicao);
                erros++;
            } else if (!t->literal && carregado && !microc_token_existe(t->nome)) {
                diag_erro(t->linha, t->coluna, "terminal %s nao existe na enumeracao "
                          "TokenType de %s", t->nome, g->tokens_externos);
                erros++;
            }
        } else if (!t->declarado) {
            diag_erro(t->linha, t->coluna, "terminal %s nao declarado em TOKENS", t->nome);
            erros++;
        }
    }
    return erros;
}

/* ------------------------------------------------------------------ */
/* A implementar                                                      */
/* ------------------------------------------------------------------ */

/* TODO(aluno): emitir um AVISO para cada nao terminal que nao pode ser
 * alcancado a partir do simbolo inicial (g->inicial). Sugestao: marque
 * o inicial e, ate um ponto fixo, marque todo nao terminal que aparece
 * na regra de um nao terminal ja marcado (expr_percorrer ajuda). */
static int verif_inalcancaveis(Gramatica *g)
{   
    /*  
    lembrete pra mim, depois tento deixar mais bonito
    retornar quantidade nao alcaçaveis
    iniciar pelo inicial
    busca em profundidade, expandindo cada nao terminal de acordo com sua regra
    se aplicado todas sua produções, nao expandir mais
    ao percorrer todos opções de n-term o que nao nao foram atingindo a partir do inicial nao é alcançavel
    usar vetor regras de nao nterm de gramatica,
    acho que nao precisa criar arvore extra apenas usar indice da lista de n-term e recursividade
    lista de nao A 
    */
    int erros, quantNTerm, *visitados, i;
    if(visitados == NULL) {
    }
    visitados = calloc(g->nnts, sizeof(int));
    erros = 0;
    quantNTerm = g->nnts;

    visitados[g->inicial] = 1;
    visitarNaoTerminal(g->inicial, g, visitados);
    
    for(i = 0; i < quantNTerm; i++) {
        if(!visitados[i] && g->nts[i].linha_def == 1) {
            erros++;
        }
    }

    free(visitados);
    (void) g;

    return erros;
    return 0;
}

/* TODO(aluno): emitir um ERRO para cada nao terminal definido que nao
 * deriva nenhuma cadeia de terminais. Ponto fixo: um nao terminal e
 * produtivo se sua regra for produtiva, onde terminais, E_VAZIO, [..] e
 * {..} sao produtivos, uma sequencia exige todos os filhos produtivos
 * e uma alternativa exige ao menos um. Retorne o numero de erros. */
static int verif_improdutivos(Gramatica *g)
{
    (void) g;
    return 0;
}

/* TODO(aluno): detectar recursao a esquerda direta (a = a ...) e
 * indireta (a = b ... ; b = a ...), inclusive atraves de prefixos
 * anulaveis (a = [ x ] a ...). Use o campo anulavel de Expr, ja
 * calculado quando esta funcao e chamada. Reporte um erro por nao
 * terminal recursivo e retorne o numero de erros. */
int verif_recursao_esquerda(Gramatica *g)
{
    (void) g;
    return 0;
}

int verif_basica(Gramatica *g, int modo_microc)
{
    int erros = 0;
    if (g->inicial < 0) {
        diag_erro(0, 0, "a gramatica nao tem nenhuma regra");
        return 1;
    }
    erros += verif_indefinidos(g);
    erros += verif_terminais(g, modo_microc);
    erros += verif_inalcancaveis(g);
    if (erros == 0)
        erros += verif_improdutivos(g);
    return erros;
}


static void visitarNaoTerminal(int indice, Gramatica *gramatica, int *visitados) {
    contextoAnalise _contexto;
    _contexto.gramatica = gramatica;
    _contexto.visitados = visitados;

    if(!visitados[indice]) {
        visitados[indice] = 1;
        
        expr_percorrer(gramatica->nts[indice].regra, analisarNoReferencia, &_contexto);
    }
}


static void analisarNoReferencia(Expr *no, void *contexto) {
    contextoAnalise *_contexto = contexto;
    if(no->tipo == E_NAOTERM) {
        visitarNaoTerminal(no->simbolo, _contexto->gramatica,_contexto->visitados);
    }
}

