/*
 * codegen.c - Geracao do analisador sintatico (etapa 5).
 *
 * Estrutura do arquivo .c gerado:
 *
 *   1. includes e conjuntos constantes (conj_0, conj_1, ...)
 *   2. estado e primitivas: avancar, match, erro_sintatico, arvore
 *   3. modo proprio: analisador lexico gerado a partir de TOKENS
 *      modo Micro C: tokens obtidos de proximo_token() (scanner do TP1)
 *   4. leitura de um arquivo de tokens (opcao --tokens do analisador)
 *   5. um procedimento parse_X() por nao terminal
 *   6. main
 *
 * Fornecido: toda a estrutura acima, os nomes dos tokens, os conjuntos,
 * as primitivas, o lexico (exceto o reconhecimento das expressoes
 * regulares) e a traducao de terminais e nao terminais.
 *
 * TODO(aluno):
 *   - gen_expr: traducao de sequencia, alternativa, [ ] e { }
 *     (esquema da secao 4 do enunciado);
 *   - gen_casamento_regex: converter uma expressao regular de TOKENS em
 *     codigo C (automato finito) que reconhece o maior prefixo;
 *   - gen_token_de_arquivo: leitura do arquivo de tokens do TP1.
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "microc_mapa.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* Emissao de texto                                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    FILE *f;
    int nivel;          /* nivel de indentacao (4 espacos por nivel) */
} Emissor;

/* Emite uma linha indentada; fmt segue printf. */
static void emit(Emissor *em, const char *fmt, ...)
{
    int i;
    va_list ap;
    for (i = 0; i < em->nivel; i++)
        fputs("    ", em->f);
    va_start(ap, fmt);
    vfprintf(em->f, fmt, ap);
    va_end(ap);
    fputc('\n', em->f);
}

/* Emite linhas fixas, sem interpretar '%'. A lista termina em NULL. */
static void emit_linhas(FILE *f, const char *const *linhas)
{
    for (; *linhas != NULL; linhas++) {
        fputs(*linhas, f);
        fputc('\n', f);
    }
}

/* Texto dentro de comentario C: evita fechar o comentario por engano. */
static void emitir_comentario(FILE *f, const char *s)
{
    for (; *s != '\0'; s++) {
        if (s[0] == '*' && s[1] == '/') {
            fputs("* /", f);
            s++;
        } else if (*s == '\n') {
            fputc(' ', f);
        } else {
            fputc(*s, f);
        }
    }
}

/* String C entre aspas, com escapes. */
static void emitir_string_c(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s != '\0'; s++) {
        unsigned char c = (unsigned char) *s;
        if (c == '"' || c == '\\')
            fprintf(f, "\\%c", c);
        else if (c == '\n')
            fputs("\\n", f);
        else if (c == '\t')
            fputs("\\t", f);
        else if (c < 32 || c > 126)
            fprintf(f, "\\%03o", c);
        else
            fputc(c, f);
    }
    fputc('"', f);
}

/* ------------------------------------------------------------------ */
/* Nomes dos tokens no codigo gerado                                  */
/* ------------------------------------------------------------------ */

static const char *const nomes_simbolos[][2] = {
    { "+", "MAIS" }, { "-", "MENOS" }, { "*", "VEZES" }, { "/", "BARRA" },
    { "%", "PORCENTO" }, { "(", "ABRE_PAR" }, { ")", "FECHA_PAR" },
    { "[", "ABRE_COL" }, { "]", "FECHA_COL" }, { "{", "ABRE_CHAVE" },
    { "}", "FECHA_CHAVE" }, { ";", "PV" }, { ",", "VIRGULA" },
    { ".", "PONTO" }, { ":", "DOIS_PONTOS" }, { "=", "IGUAL" },
    { "==", "IGUAL_IGUAL" }, { "!=", "DIFERENTE" }, { "<", "MENOR" },
    { ">", "MAIOR" }, { "<=", "MENOR_IGUAL" }, { ">=", "MAIOR_IGUAL" },
    { "&&", "E_LOGICO" }, { "||", "OU_LOGICO" }, { "!", "NAO" },
    { ":=", "ATRIBUICAO" }, { "->", "SETA" }
};

static int eh_palavra(const char *s)
{
    if (!isalpha((unsigned char) *s) && *s != '_')
        return 0;
    for (; *s != '\0'; s++)
        if (!isalnum((unsigned char) *s) && *s != '_')
            return 0;
    return 1;
}

static int nome_em_uso(const Gramatica *g, int ate, const char *nome)
{
    int i;
    for (i = 0; i < ate; i++)
        if (strcmp(g->terms[i].nome_c, nome) == 0)
            return 1;
    return 0;
}

/* Define g->terms[i].nome_c para todos os terminais. */
static void nomear_terminais(Gramatica *g, int microc)
{
    int i;
    size_t k;
    char buf[256];
    Terminal *t;

    for (i = 0; i < g->nterms; i++) {
        t = &g->terms[i];
        free(t->nome_c);
        if (i == TERM_FIM) {
            t->nome_c = xstrdup(microc ? "END_OF_FILE" : "T_FIM");
            continue;
        }
        if (microc) {
            t->nome_c = xstrdup(t->literal ? microc_nome_literal(t->nome) : t->nome);
            continue;
        }
        buf[0] = '\0';
        if (!t->literal) {
            sprintf(buf, "T_%.200s", t->nome);
        } else if (eh_palavra(t->nome) && strlen(t->nome) < 200) {
            sprintf(buf, "T_%s", t->nome);
            for (k = 2; buf[k] != '\0'; k++)
                buf[k] = (char) toupper((unsigned char) buf[k]);
        } else {
            for (k = 0; k < sizeof(nomes_simbolos) / sizeof(nomes_simbolos[0]); k++)
                if (strcmp(nomes_simbolos[k][0], t->nome) == 0)
                    sprintf(buf, "T_%s", nomes_simbolos[k][1]);
            if (buf[0] == '\0')
                sprintf(buf, "T_LIT_%d", i);
        }
        if (nome_em_uso(g, i, buf))
            sprintf(buf + strlen(buf), "_%d", i);
        t->nome_c = xstrdup(buf);
    }
}

/* ------------------------------------------------------------------ */
/* Conjuntos constantes                                               */
/* ------------------------------------------------------------------ */

#define MAX_COMPARACOES 3   /* ate 3 tokens: tok.tipo == A || ...; acima: pertence */

static Bitset *conjuntos = NULL;
static int nconj = 0, capconj = 0;

static int bitset_igual(const Bitset *a, const Bitset *b)
{
    int i;
    for (i = 0; i < a->n; i++)
        if (bitset_tem(a, i) != bitset_tem(b, i))
            return 0;
    return 1;
}

/* Registra um conjunto e devolve seu numero N (emitido como conj_N).
 * Conjuntos iguais compartilham a mesma constante. */
static int registrar_conjunto(const Bitset *b)
{
    int i;
    for (i = 0; i < nconj; i++)
        if (bitset_igual(&conjuntos[i], b))
            return i;
    if (nconj == capconj) {
        capconj = capconj ? 2 * capconj : 16;
        conjuntos = (Bitset *) xrealloc(conjuntos, (size_t) capconj * sizeof(Bitset));
    }
    bitset_iniciar(&conjuntos[nconj], b->n);
    bitset_copiar(&conjuntos[nconj], b);
    return nconj++;
}

static void liberar_conjuntos(void)
{
    int i;
    for (i = 0; i < nconj; i++)
        bitset_liberar(&conjuntos[i]);
    free(conjuntos);
    conjuntos = NULL;
    nconj = capconj = 0;
}

static void emitir_conjuntos(FILE *f, const Gramatica *g)
{
    int i, t;
    for (i = 0; i < nconj; i++) {
        fprintf(f, "static const int conj_%d[] = { ", i);
        for (t = bitset_proximo(&conjuntos[i], 0); t >= 0;
             t = bitset_proximo(&conjuntos[i], t + 1))
            fprintf(f, "%s, ", g->terms[t].nome_c);
        fputs("-1 };\n", f);
    }
}

/* Escreve a condicao "o token corrente pertence a b":
 *   tok.tipo == T_A                     (um token)
 *   tok.tipo == T_A || tok.tipo == T_B  (ate MAX_COMPARACOES)
 *   pertence(tok.tipo, conj_N)          (mais tokens) */
static void emitir_teste(FILE *f, const Gramatica *g, const Bitset *b)
{
    int t, n = bitset_tamanho(b), primeiro = 1;
    if (n == 0) {
        fputs("0", f);
    } else if (n <= MAX_COMPARACOES) {
        for (t = bitset_proximo(b, 0); t >= 0; t = bitset_proximo(b, t + 1)) {
            fprintf(f, "%stok.tipo == %s", primeiro ? "" : " || ", g->terms[t].nome_c);
            primeiro = 0;
        }
    } else {
        fprintf(f, "pertence(tok.tipo, conj_%d)", registrar_conjunto(b));
    }
}

/* Linha "<antes><teste><depois>", ex.: emit_teste(em, "while (", g, b, ") {") */
static void emit_teste(Emissor *em, const char *antes, const Gramatica *g,
                       const Bitset *b, const char *depois)
{
    int i;
    for (i = 0; i < em->nivel; i++)
        fputs("    ", em->f);
    fputs(antes, em->f);
    emitir_teste(em->f, g, b);
    fputs(depois, em->f);
    fputc('\n', em->f);
}

/* Linha "erro_sintatico(conj_N);" com N = conjunto dos tokens esperados. */
static void emit_erro(Emissor *em, const Bitset *esperados)
{
    emit(em, "erro_sintatico(conj_%d);", registrar_conjunto(esperados));
}

/* ------------------------------------------------------------------ */
/* Traducao das expressoes (esquema da secao 4 do enunciado)          */
/* ------------------------------------------------------------------ */

static void gen_expr(Emissor *em, const Gramatica *g, const Expr *e)
{
    int i;
    int indice_anulavel = -1;
    int primeira_opcao = 1;

    switch (e->tipo) {
    case E_TERM:
        emit(em, "match(%s);", g->terms[e->simbolo].nome_c);
        break;
    case E_NAOTERM:
        emit(em, "parse_%s();", g->nts[e->simbolo].nome);
        break;

    /* parte do TODO (Aparentemente feito) */
    case E_SEQ:
        i = 0;
        while (i<e->nfilhos){
            gen_expr(em, g, e->filhos[i]);
            i++;
        }
        break;
    case E_ALT:
        i = 0;
        while (i < e->nfilhos) {
            if (e->filhos[i]->anulavel) {
                indice_anulavel = i;
                break;
            }
            i++;
        }

        primeira_opcao = 1;
        i = 0;
        while (i < e->nfilhos) {
            if (i == indice_anulavel) {
                i++;
                continue;
            }

            if (primeira_opcao) {
                emit_teste(em, "if (", g, &e->filhos[i]->first, ") {");
                primeira_opcao = 0;
            } else {
                emit_teste(em, "} else if (", g, &e->filhos[i]->first, ") {");
            }

            em->nivel++;
            gen_expr(em, g, e->filhos[i]);
            em->nivel--;
            i++;
        }

        if (indice_anulavel != -1) {
            if (primeira_opcao) {
                gen_expr(em, g, e->filhos[indice_anulavel]);
            } else {
                emit(em, "} else {");
                em->nivel++;
                gen_expr(em, g, e->filhos[indice_anulavel]);
                em->nivel--;
                emit(em, "}");
            }
        } else {
            emit(em, "} else {");
            em->nivel++;
            emit_erro(em, &e->first);
            em->nivel--;
            emit(em, "}");
        }
        break;
    case E_OPC:
        emit_teste(em, "if (", g, &e->filhos[0]->first, ") {");
        em->nivel++;
        gen_expr(em, g, e->filhos[0]);
        em->nivel--;
        emit(em, "}");
        break;
    case E_REP:
        emit_teste(em, "while (", g, &e->filhos[0]->first, ") {");
        em->nivel++;
        gen_expr(em, g, e->filhos[0]);
        em->nivel--;
        emit(em, "}");
        break;
    case E_VAZIO:
        break;
    /*
     * TODO(aluno): traduzir as demais construcoes (secao 4):
     *
     *   E_SEQ   a1 a2 ... an   ->  gen_expr de cada filho, em ordem
     *   E_ALT   a1 | ... | an  ->  if (FIRST(a1)) {...} else if ... else erro
     *   E_OPC   [ a ]          ->  if (FIRST(a)) { ... }
     *   E_REP   { a }          ->  while (FIRST(a)) { ... }
     *   E_VAZIO                ->  nada
     *
     * Ferramentas disponiveis neste arquivo:
     *   emit(em, fmt, ...)                 uma linha indentada
     *   em->nivel++ / em->nivel--          indentacao dos blocos
     *   emit_teste(em, "if (", g, &x->first, ") {")
     *   emit_erro(em, &e->first)           erro_sintatico com os esperados
     * Os conjuntos FIRST de cada no foram calculados em conjuntos.c.
     * Uma alternativa anulavel nao tem teste: ela fica no else final,
     * no lugar do erro.
     */
    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Expressoes regulares de TOKENS (modo proprio)                      */
/* ------------------------------------------------------------------ */

/* Regex -> AFN (Thompson) -> AFD (subconjuntos) -> codigo C.
 * Alfabeto: bytes 1..255 (o '\0' nunca casa). '.' nao casa '\n';
 * [^...] casa '\n'. Usa apenas ctype/stdio/stdlib/string e diag_erro. */

#define RE_MAX_NFA 4096
#define RE_MAX_DFA 8192

typedef struct {
    unsigned char set[32];  /* conjunto de caracteres (se is_set) */
    int is_set;             /* 1: transicao por caractere; 0: epsilon */
    int a, b;               /* destinos (is_set usa so 'a') */
} ReEstado;

typedef struct {
    int ini, fim;
} ReFrag;

typedef struct {
    const char *p;
    ReEstado *n;
    int nn;
    const char *erro;
} ReParser;

static void re_bit_set(unsigned char *s, int c) { s[c >> 3] |= (unsigned char) (1u << (c & 7)); }
static int  re_bit_get(const unsigned char *s, int c) { return (s[c >> 3] >> (c & 7)) & 1; }

static void re_erro(ReParser *P, const char *msg){
    if (!P->erro)
        P->erro = msg;
}

static int re_novo(ReParser *P, int is_set){
    ReEstado *e;
    if (P->nn >= RE_MAX_NFA) {
        re_erro(P, "expressao grande demais");
        return 0;
    }
    e = &P->n[P->nn];
    memset(e, 0, sizeof *e);
    e->a = e->b = -1;
    e->is_set = is_set;
    return P->nn++;
}

static void re_ligar(ReParser *P, int de, int para){
    if (P->n[de].a < 0)
        P->n[de].a = para;
    else
        P->n[de].b = para;
}

static ReFrag re_frag_eps(ReParser *P){
    ReFrag f;
    f.ini = re_novo(P, 0);
    f.fim = re_novo(P, 0);
    re_ligar(P, f.ini, f.fim);
    return f;
}

static ReFrag re_frag_set(ReParser *P, const unsigned char set[32]){
    ReFrag f;
    f.ini = re_novo(P, 1);
    f.fim = re_novo(P, 0);
    memcpy(P->n[f.ini].set, set, 32);
    P->n[f.ini].a = f.fim;
    return f;
}

/* le o caractere depois de '\' */
static unsigned char re_escape(ReParser *P){
    unsigned char c;
    if (!*P->p) {
        re_erro(P, "'\\' no fim da expressao");
        return '\\';
    }
    c = (unsigned char) *P->p++;
    switch (c) {
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
    default:  return c;      /* \\ \/ \. \( \* ... = o proprio caractere */
    }
}

static int re_char_classe(ReParser *P){
    if (*P->p == '\\') {
        P->p++;
        return re_escape(P);
    }
    return (unsigned char) *P->p++;
}

static void re_classe(ReParser *P, unsigned char set[32]){
    int neg = 0, i;
    memset(set, 0, 32);
    P->p++;                                  /* '[' */
    if (*P->p == '^') {
        neg = 1;
        P->p++;
    }
    while (*P->p && *P->p != ']') {
        int lo = re_char_classe(P), hi = lo;
        if (*P->p == '-' && P->p[1] && P->p[1] != ']') {
            P->p++;
            hi = re_char_classe(P);
            if (hi < lo) {
                re_erro(P, "faixa invalida em classe");
                hi = lo;
            }
        }
        for (i = lo; i <= hi; i++)
            re_bit_set(set, i);
    }
    if (*P->p != ']')
        re_erro(P, "classe [...] sem ']'");
    else
        P->p++;
    if (neg)
        for (i = 0; i < 32; i++)
            set[i] = (unsigned char) ~set[i];
    set[0] &= (unsigned char) ~1u;           /* '\0' nunca casa */
}

static ReFrag re_alt(ReParser *P);

static ReFrag re_atomo(ReParser *P){
    unsigned char set[32];
    char c = *P->p;

    if (c == '(') {
        ReFrag f;
        P->p++;
        f = re_alt(P);
        if (*P->p != ')')
            re_erro(P, "falta ')'");
        else
            P->p++;
        return f;
    }
    if (c == '[') {
        re_classe(P, set);
        return re_frag_set(P, set);
    }
    if (c == '.') {
        int i;
        P->p++;
        memset(set, 0xFF, 32);
        set[0] &= (unsigned char) ~1u;
        i = '\n';
        set[i >> 3] &= (unsigned char) ~(1u << (i & 7));
        return re_frag_set(P, set);
    }
    if (c == '*' || c == '+' || c == '?') {
        re_erro(P, "operador sem operando");
        P->p++;
        return re_frag_eps(P);
    }
    memset(set, 0, 32);
    if (c == '\\') {
        P->p++;
        re_bit_set(set, re_escape(P));
    } else {
        re_bit_set(set, (unsigned char) c);
        P->p++;
    }
    return re_frag_set(P, set);
}

static ReFrag re_rep(ReParser *P){
    ReFrag a = re_atomo(P);

    while (*P->p == '*' || *P->p == '+' || *P->p == '?') {
        char op = *P->p++;
        int s = re_novo(P, 0), e = re_novo(P, 0);

        if (op == '*') {
            P->n[s].a = a.ini; P->n[s].b = e;
            re_ligar(P, a.fim, a.ini);
            re_ligar(P, a.fim, e);
            a.ini = s;
        } else if (op == '+') {
            re_ligar(P, a.fim, a.ini);
            re_ligar(P, a.fim, e);
        } else {                              /* '?' */
            P->n[s].a = a.ini; P->n[s].b = e;
            re_ligar(P, a.fim, e);
            a.ini = s;
        }
        a.fim = e;
    }
    return a;
}

static ReFrag re_cat(ReParser *P){
    ReFrag f = re_frag_eps(P);
    while (*P->p && *P->p != '|' && *P->p != ')') {
        ReFrag g = re_rep(P);
        re_ligar(P, f.fim, g.ini);
        f.fim = g.fim;
    }
    return f;
}

static ReFrag re_alt(ReParser *P){
    ReFrag f = re_cat(P);
    while (*P->p == '|') {
        ReFrag g;
        int s, e;
        P->p++;
        g = re_cat(P);
        s = re_novo(P, 0);
        e = re_novo(P, 0);
        P->n[s].a = f.ini; P->n[s].b = g.ini;
        re_ligar(P, f.fim, e);
        re_ligar(P, g.fim, e);
        f.ini = s;
        f.fim = e;
    }
    return f;
}

/* ---------- construcao de subconjuntos ---------- */

typedef struct {
    int nb;                  /* bytes por conjunto de estados do AFN */
    int ne, cap;             /* numero de estados do AFD / capacidade */
    unsigned char *conj;     /* ne * nb */
    int *trans;              /* ne * 256, -1 = sem transicao */
    char *final;             /* ne */
} ReDfa;

static void re_fecho(const ReParser *P, unsigned char *set, int *pilha){
    int i, topo = 0;
    for (i = 0; i < P->nn; i++)
        if (re_bit_get(set, i))
            pilha[topo++] = i;
    while (topo > 0) {
        const ReEstado *e = &P->n[pilha[--topo]];
        int k, alvo[2];
        if (e->is_set)
            continue;
        alvo[0] = e->a;
        alvo[1] = e->b;
        for (k = 0; k < 2; k++)
            if (alvo[k] >= 0 && !re_bit_get(set, alvo[k])) {
                re_bit_set(set, alvo[k]);
                pilha[topo++] = alvo[k];
            }
    }
}

/* devolve indice do estado do AFD com esse conjunto (criando se preciso);
 * -2 em falta de memoria / limite */
static int re_dfa_estado(ReDfa *D, const unsigned char *set, int final){
    int i;
    for (i = 0; i < D->ne; i++)
        if (memcmp(D->conj + (size_t) i * D->nb, set, (size_t) D->nb) == 0)
            return i;
    if (D->ne >= RE_MAX_DFA)
        return -2;
    if (D->ne >= D->cap) {
        int ncap = D->cap ? D->cap * 2 : 16;
        unsigned char *c = realloc(D->conj, (size_t) ncap * D->nb);
        int *t;
        char *fi;
        if (!c) return -2;
        D->conj = c;
        t = realloc(D->trans, (size_t) ncap * 256 * sizeof *t);
        if (!t) return -2;
        D->trans = t;
        fi = realloc(D->final, (size_t) ncap);
        if (!fi) return -2;
        D->final = fi;
        D->cap = ncap;
    }
    memcpy(D->conj + (size_t) D->ne * D->nb, set, (size_t) D->nb);
    memset(D->trans + (size_t) D->ne * 256, 0xFF, 256 * sizeof(int)); /* -1 */
    D->final[D->ne] = (char) final;
    return D->ne++;
}

static void re_emite_chr(FILE *f, int c){
    if (c == '\\')
        fputs("'\\\\'", f);
    else if (c == '\'')
        fputs("'\\''", f);
    else if (isprint(c))
        fprintf(f, "'%c'", c);
    else
        fprintf(f, "%d", c);
}

/* Emite a funcao
 *     static size_t <nome>(const char *s)
 * que devolve o comprimento do MAIOR prefixo de s reconhecido pela
 * expressao regular, ou 0 se nenhum prefixo for reconhecido.
 * Retorna 0 se a expressao for valida (senao reporte com diag_erro). */
static int gen_casamento_regex(FILE *f, const char *nome, const char *regex){
    ReParser P;
    ReFrag nfa;
    ReDfa D;
    unsigned char *cur = NULL, *tmp = NULL;
    int *pilha = NULL;
    int d, c, ret = -1, algum_final = 0;

    memset(&D, 0, sizeof D);
    P.p = regex;
    P.nn = 0;
    P.erro = NULL;
    P.n = malloc(RE_MAX_NFA * sizeof *P.n);
    if (!P.n)
        return -1;

    nfa = re_alt(&P);
    if (!P.erro && *P.p)
        re_erro(&P, "')' sem '(' correspondente");
    if (P.erro) {
        diag_erro(0, 0, "expressao regular '%s': %s", regex, P.erro);
        goto fim;
    }

    D.nb = (P.nn + 7) / 8;
    cur = malloc((size_t) D.nb);
    tmp = malloc((size_t) D.nb);
    pilha = malloc((size_t) P.nn * sizeof *pilha);
    if (!cur || !tmp || !pilha)
        goto fim;

    memset(cur, 0, (size_t) D.nb);
    re_bit_set(cur, nfa.ini);
    re_fecho(&P, cur, pilha);
    if (re_dfa_estado(&D, cur, re_bit_get(cur, nfa.fim)) < 0)
        goto fim;

    for (d = 0; d < D.ne; d++) {
        memcpy(cur, D.conj + (size_t) d * D.nb, (size_t) D.nb); /* conj pode realocar */
        for (c = 1; c < 256; c++) {
            int i, alvo, vazio = 1;
            memset(tmp, 0, (size_t) D.nb);
            for (i = 0; i < P.nn; i++)
                if (re_bit_get(cur, i) && P.n[i].is_set &&
                    re_bit_get(P.n[i].set, c))
                    re_bit_set(tmp, P.n[i].a);
            for (i = 0; i < D.nb; i++)
                if (tmp[i]) { vazio = 0; break; }
            if (vazio)
                continue;
            re_fecho(&P, tmp, pilha);
            alvo = re_dfa_estado(&D, tmp, re_bit_get(tmp, nfa.fim));
            if (alvo < 0) {
                diag_erro(0, 0, "automato grande demais para a expressao regular '%s'", regex);
                goto fim;
            }
            D.trans[(size_t) d * 256 + c] = alvo;
        }
    }

    /* ---------- emissao do codigo C ---------- */
    fprintf(f, "static size_t %s(const char *s)\n{\n", nome);
    fputs("    /* automato para ", f);
    emitir_comentario(f, regex);
    fputs(" */\n", f);
    fputs("    size_t i = 0, ultimo = 0;   /* ultimo = fim do maior casamento */\n"
          "    int estado = 0;\n\n"
          "    for (;;) {\n"
          "        unsigned char c = (unsigned char) s[i];\n"
          "        int prox;\n\n"
          "        switch (estado) {\n", f);

    for (d = 0; d < D.ne; d++) {
        int primeiro = 1;
        fprintf(f, "        case %d:\n", d);
        c = 1;
        while (c < 256) {
            int t = D.trans[(size_t) d * 256 + c], lo = c, hi;
            if (t < 0) { c++; continue; }
            while (c + 1 < 256 && D.trans[(size_t) d * 256 + c + 1] == t)
                c++;
            hi = c++;
            fputs(primeiro ? "            if (" : "            else if (", f);
            primeiro = 0;
            if (lo == hi) {
                fputs("c == ", f);
                re_emite_chr(f, lo);
            } else {
                fputs("c >= ", f);
                re_emite_chr(f, lo);
                if (hi < 255) {
                    fputs(" && c <= ", f);
                    re_emite_chr(f, hi);
                }
            }
            fprintf(f, ") prox = %d;\n", t);
        }
        fputs(primeiro ? "            prox = -1;\n" : "            else prox = -1;\n", f);
        fputs("            break;\n", f);
        if (D.final[d])
            algum_final = 1;
    }
    fputs("        default:\n            prox = -1;\n            break;\n        }\n\n"
          "        if (prox < 0)\n            return ultimo;\n"
          "        estado = prox;\n        i++;\n", f);

    if (algum_final) {
        fputs("\n        switch (estado) {\n", f);
        for (d = 0; d < D.ne; d++)
            if (D.final[d])
                fprintf(f, "        case %d:\n", d);
        fputs("            ultimo = i;\n            break;\n"
              "        default:\n            break;\n        }\n", f);
    }
    fputs("    }\n}\n\n", f);
    ret = 0;

fim:
    free(P.n);
    free(cur);
    free(tmp);
    free(pilha);
    free(D.conj);
    free(D.trans);
    free(D.final);
    return ret;
}

/* ------------------------------------------------------------------ */
/* Leitura do arquivo de tokens (opcao --tokens do analisador gerado) */
/* ------------------------------------------------------------------ */

/* Emite "static Token token_de_arquivo(void)", que le o proximo token
 * do arquivo aberto em arquivo_tokens, no formato gravado pelo lexer do
 * TP1 (livro-texto, secao 3.3.2):
 *
 *   Token: tipo = <numero>, lexema = '<texto>', linha = <numero>
 *
 * Ao chegar ao fim do arquivo devolve um token TOKEN_FIM. */
static void gen_token_de_arquivo(FILE *f, int microc){
    /* Cada linha do arquivo tem o formato
     *     Token: tipo = <numero>, lexema = '<texto>', linha = <numero>
     * O lexema pode conter espacos, virgulas e ate aspas, entao o texto
     * e' o que fica entre "lexema = '" e a ULTIMA ocorrencia de
     * "', linha = "; os numeros sao lidos com sscanf. */
    static const char *const ini[] = {
        "static Token token_de_arquivo(void)",
        "{",
        "    static const char marca[] = \"', linha = \";",
        "    static int numero_linha = 0, ultima_linha = 0;",
        "    char buf[1024];",
        "    Token t;",
        "",
        "    memset(&t, 0, sizeof t);",
        "    while (fgets(buf, sizeof buf, arquivo_tokens) != NULL) {",
        "        int tipo = 0, pos = 0, linha_tok = 0;",
        "        size_t len, n;",
        "        char *ini, *fim = NULL, *p;",
        "",
        "        numero_linha++;",
        "        for (p = buf; *p == ' ' || *p == '\\t' || *p == '\\r' || *p == '\\n'; p++)",
        "            ;",
        "        if (*p == '\\0')",
        "            continue;                   /* linha em branco */",
        "        ini = buf;",
        "        if (sscanf(buf, \" Token: tipo = %d, lexema = '%n\", &tipo, &pos) == 1",
        "            && pos > 0) {",
        "            ini = buf + pos;",
        "            for (p = strstr(ini, marca); p != NULL; p = strstr(p + 1, marca))",
        "                fim = p;                /* ultima ocorrencia */",
        "        }",
        "        if (fim == NULL || sscanf(fim + strlen(marca), \"%d\", &linha_tok) != 1) {",
        "            fprintf(stderr, \"arquivo de tokens, linha %d: linha malformada\\n\",",
        "                    numero_linha);",
        "            exit(EXIT_FAILURE);",
        "        }",
        "        len = (size_t) (fim - ini);",
        "        n = len < sizeof t.lexema ? len : sizeof t.lexema - 1;",
        "        memcpy(t.lexema, ini, n);",
        "        t.lexema[n] = '\\0';",
        NULL
    };
    static const char *const fim[] = {
        "        t.linha = linha_tok;",
        "        ultima_linha = linha_tok;",
        "        return t;",
        "    }",
        "    t.tipo = TOKEN_FIM;",
        "    t.linha = ultima_linha;",
        "    return t;",
        "}",
        "",
        NULL
    };
    emit_linhas(f, ini);
    /* no Micro C o campo tipo e' o enum TokenType do scanner do TP1 */
    fputs(microc ? "        t.tipo = (TokenType) tipo;\n"
                 : "        t.tipo = tipo;\n", f);
    emit_linhas(f, fim);
}

/* ------------------------------------------------------------------ */
/* Partes fixas do codigo gerado                                      */
/* ------------------------------------------------------------------ */

static const char *const rt_estado[] = {
    "/* ---- estado do analisador ---- */",
    "",
    "static Token tok;                  /* token corrente (lookahead) */",
    "static int imprimir_arvore = 0;    /* opcao --tree */",
    "static int nivel_arvore = 0;",
    "static FILE *arquivo_tokens = NULL; /* opcao --tokens */",
    "",
    "static Token token_de_arquivo(void);",
    "",
    NULL
};

static const char *const rt_primitivas[] = {
    "/* ---- primitivas ---- */",
    "",
    "int pertence(int tipo, const int *conjunto)",
    "{",
    "    for (; *conjunto != -1; conjunto++)",
    "        if (*conjunto == tipo)",
    "            return 1;",
    "    return 0;",
    "}",
    "",
    "/* Reporta o erro com os tokens esperados (lista terminada em -1) e encerra. */",
    "void erro_sintatico(const int *esperados)",
    "{",
    "    int i;",
    "    escrever_posicao();",
    "    if ((int) tok.tipo == TOKEN_FIM)",
    "        fprintf(stderr, \"encontrado fim de arquivo, \");",
    "    else",
    "        fprintf(stderr, \"encontrado '%s', \", tok.lexema);",
    "    if (esperados[0] != -1 && esperados[1] == -1) {",
    "        fprintf(stderr, \"esperado %s\\n\", nome_token(esperados[0]));",
    "    } else {",
    "        fprintf(stderr, \"esperado um de {\");",
    "        for (i = 0; esperados[i] != -1; i++)",
    "            fprintf(stderr, \"%s%s\", i > 0 ? \", \" : \"\", nome_token(esperados[i]));",
    "        fprintf(stderr, \"}\\n\");",
    "    }",
    "    exit(EXIT_FAILURE);",
    "}",
    "",
    "static void recuo(void)",
    "{",
    "    int i;",
    "    for (i = 0; i < nivel_arvore; i++)",
    "        fputs(\"  \", stdout);",
    "}",
    "",
    "void arvore_entra(const char *nao_terminal)",
    "{",
    "    if (imprimir_arvore) {",
    "        recuo();",
    "        printf(\"%s\\n\", nao_terminal);",
    "    }",
    "    nivel_arvore++;",
    "}",
    "",
    "void arvore_sai(void)",
    "{",
    "    nivel_arvore--;",
    "}",
    "",
    "/* Consome o token corrente se for do tipo esperado; senao, erro. */",
    "void match(int esperado)",
    "{",
    "    int lista[2];",
    "    if ((int) tok.tipo != esperado) {",
    "        lista[0] = esperado;",
    "        lista[1] = -1;",
    "        erro_sintatico(lista);",
    "    }",
    "    if (SUPORTE_ARVORE && imprimir_arvore) {",
    "        recuo();",
    "        if (nome_token(tok.tipo)[0] == '\\'')",
    "            printf(\"%s\\n\", nome_token(tok.tipo));",
    "        else",
    "            printf(\"%s '%s'\\n\", nome_token(tok.tipo), tok.lexema);",
    "    }",
    "    avancar();",
    "}",
    "",
    NULL
};

/* Lexico proprio: partes fixas antes e depois das funcoes de regex. */
static const char *const rt_lexico_estado[] = {
    "/* ---- analisador lexico gerado a partir de TOKENS ---- */",
    "",
    "static char *fonte = NULL;",
    "static size_t pos = 0;",
    "static int linha = 1, coluna = 1;",
    "",
    "static char *ler_tudo(FILE *f)",
    "{",
    "    size_t cap = 4096, n = 0, lidos;",
    "    char *buf = (char *) malloc(cap), *novo;",
    "    if (buf == NULL) {",
    "        fprintf(stderr, \"memoria insuficiente\\n\");",
    "        exit(EXIT_FAILURE);",
    "    }",
    "    while ((lidos = fread(buf + n, 1, cap - n - 1, f)) > 0) {",
    "        n += lidos;",
    "        if (cap - n - 1 == 0) {",
    "            cap *= 2;",
    "            novo = (char *) realloc(buf, cap);",
    "            if (novo == NULL) {",
    "                fprintf(stderr, \"memoria insuficiente\\n\");",
    "                exit(EXIT_FAILURE);",
    "            }",
    "            buf = novo;",
    "        }",
    "    }",
    "    buf[n] = '\\0';",
    "    return buf;",
    "}",
    "",
    "static void consumir(size_t n)",
    "{",
    "    for (; n > 0 && fonte[pos] != '\\0'; n--, pos++) {",
    "        if (fonte[pos] == '\\n') {",
    "            linha++;",
    "            coluna = 1;",
    "        } else {",
    "            coluna++;",
    "        }",
    "    }",
    "}",
    "",
    NULL
};

static void escrever_cabecalho_c(FILE *f, const OpcoesGeracao *op)
{
    fputs("/*\n * Analisador sintatico descendente recursivo gerado a partir de ", f);
    emitir_comentario(f, op->origem);
    fputs(".\n * Arquivo gerado automaticamente: nao edite.\n */\n", f);
}

/* ------------------------------------------------------------------ */
/* Arquivo .h                                                         */
/* ------------------------------------------------------------------ */

static int gerar_h(const Gramatica *g, const OpcoesGeracao *op)
{
    char *caminho = (char *) xmalloc(strlen(op->prefixo) + 3);
    FILE *f;
    int i;

    sprintf(caminho, "%s.h", op->prefixo);
    f = fopen(caminho, "w");
    if (f == NULL) {
        diag_erro(0, 0, "nao foi possivel criar %s", caminho);
        free(caminho);
        return -1;
    }
    escrever_cabecalho_c(f, op);
    fputs("#ifndef PARSER_GERADO_H\n#define PARSER_GERADO_H\n\n", f);
    if (op->microc) {
        fputs("#include \"scanner.h\"   /* scanner do TP1: Token, TokenType, proximo_token */\n\n", f);
        fputs("#define TOKEN_FIM END_OF_FILE\n\n", f);
    } else {
        fputs("typedef enum {\n", f);
        for (i = 0; i < g->nterms; i++) {
            fprintf(f, "    %s%s   /* ", g->terms[i].nome_c, i + 1 < g->nterms ? "," : "");
            emitir_comentario(f, g->terms[i].exibicao);
            fputs(" */\n", f);
        }
        fputs("} TipoToken;\n\n", f);
        fputs("typedef struct {\n    int tipo;\n    char lexema[100];\n"
              "    int linha;\n    int coluna;\n} Token;\n\n", f);
        fputs("#define TOKEN_FIM T_FIM\n\n", f);
    }
    fputs("const char *nome_token(int tipo);\n", f);
    fputs("int  pertence(int tipo, const int *conjunto);\n", f);
    fputs("void avancar(void);\n", f);
    fputs("void match(int esperado);\n", f);
    fputs("void erro_sintatico(const int *esperados);\n", f);
    fputs("void arvore_entra(const char *nao_terminal);\n", f);
    fputs("void arvore_sai(void);\n\n", f);
    for (i = 0; i < g->nnts; i++)
        if (g->nts[i].regra != NULL)
            fprintf(f, "void parse_%s(void);\n", g->nts[i].nome);
    fputs("\n#endif\n", f);
    fclose(f);
    free(caminho);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Arquivo .c                                                         */
/* ------------------------------------------------------------------ */

static void gerar_nome_token(FILE *f, const Gramatica *g)
{
    int i;
    fputs("const char *nome_token(int tipo)\n{\n    switch (tipo) {\n", f);
    for (i = 0; i < g->nterms; i++) {
        fprintf(f, "    case %s: return ", g->terms[i].nome_c);
        emitir_string_c(f, i == TERM_FIM ? "fim de arquivo" : g->terms[i].exibicao);
        fputs(";\n", f);
    }
    fputs("    default: return \"token desconhecido\";\n    }\n}\n\n", f);
}

static void gerar_posicao_e_avancar(FILE *f, int microc)
{
    fputs("static void escrever_posicao(void)\n{\n", f);
    if (microc)
        fputs("    fprintf(stderr, \"linha %d: \", tok.linha);\n", f);
    else
        fputs("    fprintf(stderr, \"linha %d, coluna %d: \", tok.linha, tok.coluna);\n", f);
    fputs("}\n\n", f);

    fputs("/* Le o proximo token para tok. */\nvoid avancar(void)\n{\n", f);
    fputs("    if (arquivo_tokens != NULL)\n        tok = token_de_arquivo();\n    else\n", f);
    if (microc) {
        fputs("        tok = proximo_token();\n", f);
        fputs("    if (tok.tipo == UNDEF) {\n", f);
        fputs("        fprintf(stderr, \"linha %d: erro lexico: %s\\n\", tok.linha, tok.lexema);\n", f);
        fputs("        exit(EXIT_FAILURE);\n    }\n", f);
    } else {
        fputs("        tok = proximo_token_gerado();\n", f);
    }
    fputs("}\n\n", f);
}

static int gerar_lexico(FILE *f, const Gramatica *g)
{
    int i, nlit = 0, erros = 0;
    char nome[300];

    emit_linhas(f, rt_lexico_estado);

    for (i = 0; i < g->nskips; i++) {
        sprintf(nome, "casa_skip_%d", i);
        fputs("/* SKIP: ", f);
        emitir_comentario(f, g->skips[i]);
        fputs(" */\n", f);
        if (gen_casamento_regex(f, nome, g->skips[i]) != 0)
            erros++;
    }
    for (i = 1; i < g->nterms; i++) {
        if (g->terms[i].literal) {
            nlit++;
            continue;
        }
        sprintf(nome, "casa_%.250s", g->terms[i].nome_c);
        fprintf(f, "/* %s: ", g->terms[i].nome);
        emitir_comentario(f, g->terms[i].regex ? g->terms[i].regex : "");
        fputs(" */\n", f);
        if (gen_casamento_regex(f, nome, g->terms[i].regex ? g->terms[i].regex : "")
            != 0)
            erros++;
    }

    /* literais: comparacao direta, com prioridade em caso de empate */
    fputs("static const char *const texto_literais[] = {\n", f);
    for (i = 1; i < g->nterms; i++)
        if (g->terms[i].literal) {
            fputs("    ", f);
            emitir_string_c(f, g->terms[i].nome);
            fputs(",\n", f);
        }
    fputs("    NULL\n};\n\nstatic const int tipo_literais[] = {\n", f);
    for (i = 1; i < g->nterms; i++)
        if (g->terms[i].literal)
            fprintf(f, "    %s,\n", g->terms[i].nome_c);
    fputs("    -1\n};\n\n", f);

    fputs("/* Reconhece o maior prefixo da entrada; em caso de empate vence\n"
          " * o literal (palavras reservadas tem prioridade sobre ID). */\n", f);
    fputs("static Token proximo_token_gerado(void)\n{\n", f);
    fputs("    Token t;\n    size_t n, melhor;\n    int i, tipo, pulou;\n\n", f);
    fputs("    do {\n        pulou = 0;\n", f);
    for (i = 0; i < g->nskips; i++)
        fprintf(f, "        if ((n = casa_skip_%d(fonte + pos)) > 0) {\n"
                   "            consumir(n);\n            pulou = 1;\n        }\n", i);
    fputs("    } while (pulou);\n\n", f);
    fputs("    memset(&t, 0, sizeof t);\n", f);
    fputs("    t.linha = linha;\n    t.coluna = coluna;\n", f);
    fputs("    if (fonte[pos] == '\\0') {\n        t.tipo = T_FIM;\n        return t;\n    }\n", f);
    fputs("    melhor = 0;\n    tipo = -1;\n", f);
    fputs("    for (i = 0; texto_literais[i] != NULL; i++) {\n", f);
    fputs("        n = strlen(texto_literais[i]);\n", f);
    fputs("        if (n > melhor && strncmp(fonte + pos, texto_literais[i], n) == 0) {\n", f);
    fputs("            melhor = n;\n            tipo = tipo_literais[i];\n        }\n    }\n", f);
    for (i = 1; i < g->nterms; i++)
        if (!g->terms[i].literal)
            fprintf(f, "    if ((n = casa_%s(fonte + pos)) > melhor) {\n"
                       "        melhor = n;\n        tipo = %s;\n    }\n",
                    g->terms[i].nome_c, g->terms[i].nome_c);
    fputs("    if (melhor == 0) {\n", f);
    fputs("        fprintf(stderr, \"linha %d, coluna %d: erro lexico: caractere "
          "invalido '%c'\\n\",\n                linha, coluna, fonte[pos]);\n", f);
    fputs("        exit(EXIT_FAILURE);\n    }\n", f);
    fputs("    n = melhor < sizeof t.lexema ? melhor : sizeof t.lexema - 1;\n", f);
    fputs("    memcpy(t.lexema, fonte + pos, n);\n    t.lexema[n] = '\\0';\n", f);
    fputs("    t.tipo = tipo;\n    consumir(melhor);\n    return t;\n}\n\n", f);
    (void) nlit;
    return erros;
}

static void gerar_procedimentos(FILE *f, const Gramatica *g, const OpcoesGeracao *op)
{
    int i;
    Emissor em;
    em.f = f;
    em.nivel = 0;

    fputs("/* ---- procedimentos (um por nao terminal) ---- */\n\n", f);
    for (i = 0; i < g->nnts; i++) {
        if (g->nts[i].regra == NULL)
            continue;
        fprintf(f, "/* %s = ", g->nts[i].nome);
        {
            /* a regra em EBNF, sem "* /" */
            FILE *tmp = tmpfile();
            int c;
            if (tmp != NULL) {
                char buf[2] = { 0, 0 };
                expr_imprimir(tmp, g, g->nts[i].regra);
                rewind(tmp);
                while ((c = fgetc(tmp)) != EOF) {
                    buf[0] = (char) c;
                    emitir_comentario(f, buf);
                }
                fclose(tmp);
            }
        }
        fputs(" ; */\n", f);
        fprintf(f, "void parse_%s(void)\n{\n", g->nts[i].nome);
        em.nivel = 1;
        if (op->arvore)
            emit(&em, "arvore_entra(\"%s\");", g->nts[i].nome);
        gen_expr(&em, g, g->nts[i].regra);
        if (op->arvore)
            emit(&em, "arvore_sai();");
        fputs("}\n\n", f);
    }
}

static void gerar_main(FILE *f, const Gramatica *g, const OpcoesGeracao *op)
{
    fputs("/* ---- programa principal ---- */\n\n", f);
    fputs("int main(int argc, char **argv)\n{\n", f);
    fputs("    int i, fim[2];\n    const char *arquivo = NULL, *tokens = NULL;\n", f);
    fputs("    FILE *entrada = stdin;\n\n", f);
    fputs("    for (i = 1; i < argc; i++) {\n", f);
    fputs("        if (strcmp(argv[i], \"--tree\") == 0) {\n", f);
    fputs("            imprimir_arvore = 1;\n", f);
    fputs("        } else if (strcmp(argv[i], \"--tokens\") == 0 && i + 1 < argc) {\n", f);
    fputs("            tokens = argv[++i];\n", f);
    fputs("        } else if (argv[i][0] == '-' && argv[i][1] != '\\0') {\n", f);
    fputs("            fprintf(stderr, \"uso: %s [--tree] [--tokens arquivo.tok | "
          "programa]\\n\", argv[0]);\n", f);
    fputs("            return 2;\n        } else {\n            arquivo = argv[i];\n"
          "        }\n    }\n", f);
    fputs("    if (imprimir_arvore && !SUPORTE_ARVORE)\n", f);
    fputs("        fprintf(stderr, \"aviso: gere o analisador com --tree para "
          "imprimir a arvore\\n\");\n\n", f);
    fputs("    if (tokens != NULL) {\n", f);
    fputs("        arquivo_tokens = fopen(tokens, \"r\");\n", f);
    fputs("        if (arquivo_tokens == NULL) {\n", f);
    fputs("            fprintf(stderr, \"nao foi possivel abrir %s\\n\", tokens);\n", f);
    fputs("            return 2;\n        }\n    } else {\n", f);
    fputs("        if (arquivo != NULL && (entrada = fopen(arquivo, \"r\")) == NULL) {\n", f);
    fputs("            fprintf(stderr, \"nao foi possivel abrir %s\\n\", arquivo);\n", f);
    fputs("            return 2;\n        }\n", f);
    if (op->microc)
        fputs("        scanner_iniciar(entrada);\n", f);
    else
        fputs("        fonte = ler_tudo(entrada);\n", f);
    fputs("    }\n\n", f);
    fputs("    avancar();\n", f);
    fprintf(f, "    parse_%s();\n", g->nts[g->inicial].nome);
    fputs("    if (tok.tipo != TOKEN_FIM) {\n", f);
    fputs("        fim[0] = TOKEN_FIM;\n        fim[1] = -1;\n", f);
    fputs("        erro_sintatico(fim);\n    }\n", f);
    fputs("    printf(\"entrada aceita\\n\");\n\n", f);
    fputs("    if (arquivo_tokens != NULL) {\n        fclose(arquivo_tokens);\n", f);
    fputs("    } else {\n", f);
    if (op->microc)
        fputs("        scanner_finalizar();\n", f);
    else
        fputs("        free(fonte);\n", f);
    fputs("        if (entrada != stdin)\n            fclose(entrada);\n    }\n", f);
    fputs("    return 0;\n}\n", f);
}

static const char *nome_base(const char *caminho)
{
    const char *b = strrchr(caminho, '/');
    return b != NULL ? b + 1 : caminho;
}

static int gerar_c(const Gramatica *g, const OpcoesGeracao *op)
{
    char *caminho = (char *) xmalloc(strlen(op->prefixo) + 3);
    FILE *f, *corpo;
    int c, erros = 0;

    /* Os procedimentos sao gerados primeiro num arquivo temporario porque
     * registram os conjuntos (conj_N), que precisam vir antes no .c. */
    corpo = tmpfile();
    if (corpo == NULL) {
        diag_erro(0, 0, "nao foi possivel criar arquivo temporario");
        free(caminho);
        return -1;
    }
    gerar_procedimentos(corpo, g, op);

    sprintf(caminho, "%s.c", op->prefixo);
    f = fopen(caminho, "w");
    if (f == NULL) {
        diag_erro(0, 0, "nao foi possivel criar %s", caminho);
        fclose(corpo);
        free(caminho);
        return -1;
    }
    escrever_cabecalho_c(f, op);
    fputs("#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n", f);
    fprintf(f, "#include \"%s.h\"\n\n", nome_base(op->prefixo));
    fprintf(f, "#define SUPORTE_ARVORE %d\n\n", op->arvore ? 1 : 0);

    fputs("/* ---- conjuntos de tokens ---- */\n\n", f);
    emitir_conjuntos(f, g);
    fputc('\n', f);

    emit_linhas(f, rt_estado);
    if (!op->microc)
        erros += gerar_lexico(f, g);
    gerar_nome_token(f, g);
    gerar_posicao_e_avancar(f, op->microc);
    emit_linhas(f, rt_primitivas);
    gen_token_de_arquivo(f, op->microc);

    rewind(corpo);
    while ((c = fgetc(corpo)) != EOF)
        fputc(c, f);
    fclose(corpo);

    gerar_main(f, g, op);
    fclose(f);
    free(caminho);
    return erros == 0 ? 0 : -1;
}

int codegen_gerar(Gramatica *g, const OpcoesGeracao *op)
{
    int r;
    nomear_terminais(g, op->microc);
    r = gerar_h(g, op);
    if (r == 0)
        r = gerar_c(g, op);
    liberar_conjuntos();
    return r;
}