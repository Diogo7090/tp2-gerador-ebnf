/*
 * microc_mapa.c - Correspondencia com os tokens de Micro C (ver microc_mapa.h).
 *
 * A tabela de literais segue o tokens.h do livro-texto (secao 2.3.1):
 *
 *   UNDEF, ID, END_OF_FILE, INTEGERCONST, CHARCONST, STRINGCONST,
 *   PLUS, MINUS, MUL, DIV, MOD, EQ, NEQ, LT, GT, LEQ, GEQ, AND, OR, NOT,
 *   ASSIGN, SEMICOLON, COMMA, LPAREN, RPAREN, LBRACE, RBRACE,
 *   LBRACKET, RBRACKET, MAIN, IF, ELSE, FOR, RETURN, INT, CHAR, PRINT
 *
 * Se o seu tokens.h do TP1 usar outros nomes, ajuste a tabela.
 *
 * TODO(aluno): implementar microc_carregar_tokens_h.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "microc_mapa.h"
#include "util.h"

static const char *const literais[][2] = {
    { "+",  "PLUS" },      { "-",  "MINUS" },     { "*",  "MUL" },
    { "/",  "DIV" },       { "%",  "MOD" },
    { "==", "EQ" },        { "!=", "NEQ" },       { "<",  "LT" },
    { ">",  "GT" },        { "<=", "LEQ" },       { ">=", "GEQ" },
    { "&&", "AND" },       { "||", "OR" },        { "!",  "NOT" },
    { "=",  "ASSIGN" },    { ";",  "SEMICOLON" }, { ",",  "COMMA" },
    { "(",  "LPAREN" },    { ")",  "RPAREN" },    { "{",  "LBRACE" },
    { "}",  "RBRACE" },    { "[",  "LBRACKET" },  { "]",  "RBRACKET" },
    { "main", "MAIN" },    { "if", "IF" },        { "else", "ELSE" },
    { "for", "FOR" },      { "return", "RETURN" },{ "int", "INT" },
    { "char", "CHAR" },    { "print", "PRINT" }
};

static char **nomes = NULL;
static int nnomes = 0, capnomes = 0;

const char *microc_nome_literal(const char *literal)
{
    size_t i;
    for (i = 0; i < sizeof(literais) / sizeof(literais[0]); i++)
        if (strcmp(literais[i][0], literal) == 0)
            return literais[i][1];
    return NULL;
}

static void adicionar_nome(const char *s, size_t n)
{
    if (nnomes == capnomes) {
        capnomes = capnomes ? 2 * capnomes : 64;
        nomes = (char **) xrealloc(nomes, (size_t) capnomes * sizeof(char *));
    }
    nomes[nnomes++] = xstrndup(s, n);
}

/* TODO(aluno): abrir o arquivo (ler_arquivo ajuda), localizar a
 * enumeracao "typedef enum { ... } TokenType;" e chamar adicionar_nome
 * para cada identificador da lista, ignorando comentarios e valores
 * explicitos ("X = 3"). Retorne 0 em caso de sucesso.
 * Enquanto nao implementada, a verificacao dos nomes e pulada. */
int microc_carregar_tokens_h(const char *caminho)
{   
    char *conteudo = NULL;
    char *charc = NULL;
    int ehIntEnum = 0;

    if(caminho == NULL) {
        return -1;
    }

    conteudo = ler_arquivo(caminho);
    if(conteudo == NULL) {
        return -1;
    }

    charc = conteudo;

    while(*charc != '\0') {
        if(*charc == '/' && *(charc + 1) == '/') {
            while(*charc != '\0' && *(charc+1) != '\n') {
                charc++;
            }
            continue;
        }

        if(*charc == '/' && *(charc + 1) == '*') {
            charc+=2;
            while(charc != NULL && (*charc != '*' && *(charc + 1) != '/')) {
                charc++;
            }

            if(*charc != '\0') {
                charc++;
            }
            continue;
        }

        if(strncmp(charc, "typedef", 7) == 0) {
            while(*charc != '\0' && isspace((unsigned char)*charc)) {
                charc++;
            }
            if(strncmp(charc, "enum", 4) == 0) {
                while(*charc != '\0' && isspace((unsigned char)*charc)) {
                    charc++;
                }
                if(*charc == '{') {
                    ehIntEnum = 1;
                }
            }
            continue;
        }

        if(*charc == '}') {
            ehIntEnum = 0;
            charc++;
            continue;
        }

        if(isalpha((unsigned long) *charc) && ehIntEnum == 1) {
            char *inicio = charc;
            int local = 0;

            while(isalpha((unsigned long) *charc)) {
                charc++;
            }
            local = charc - inicio;
            adicionar_nome(inicio, local);

            while(*charc != '\0' && isspace((unsigned char)*charc)) {
                charc++;
            }
            
            if(*charc == '=') {
                charc++;

                while(*charc != '\0' && *charc != ',' && *charc != '}' ) {
                    charc++;
                }
            }
            if(*charc == ',') {
                charc++;
            }

            continue;
        }

        while(*charc != '\0' && isspace((unsigned char)*charc)) {
            charc++;
        }

        charc++;

    }
    free(conteudo);

    return 0;
}

int microc_token_existe(const char *nome)
{
    int i;
    for (i = 0; i < nnomes; i++)
        if (strcmp(nomes[i], nome) == 0)
            return 1;
    return 0;
}

void microc_liberar(void)
{
    int i;
    for (i = 0; i < nnomes; i++)
        free(nomes[i]);
    free(nomes);
    nomes = NULL;
    nnomes = capnomes = 0;
}
