/* 
 * microc.flex 
 * 
 * Esqueleto do analisador lexico (scanner) para a linguagem Micro C. 
 * Disciplina: Compiladores I - FACOM 
 * 
 * Este arquivo NAO esta completo. Partes do reconhecimento de tokens 
 * foram implementadas apenas como EXEMPLO, para orienta-lo(a) sobre o 
 * padrao a seguir. As demais estao marcadas com "TODO(aluno)" e devem 
 * ser completadas por voce. 
 * 
 * Compilacao: 
 *      flex microc.flex 
 *      gcc lex.yy.c -o lexer 
 * 
 * Uso: 
 *      ./lexer test.mc 
 */ 
 
%{ 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdbool.h>
#include "scanner01.h"
 
/* --------------------------------------------------------------------- 
 * 1. VOCABULARIO DE TOKENS (equivalente a tokens.h) 
 * ------------------------------------------------------------------- */ 
 
typedef enum { 
    /* Tokens fundamentais */ 
    UNDEF, 
    ID, 
    END_OF_FILE, 
 
    /* Constantes literais */ 
    INTEGERCONST, 
    CHARCONST, 
    STRINGCONST, 
 
    /* Operadores aritmeticos */ 
    PLUS, MINUS, MUL, DIV, MOD, 
 
    /* Operadores relacionais e logicos */ 
    EQ, NEQ, LT, GT, LEQ, GEQ, AND, OR, NOT, 
 
    /* Simbolos de atribuicao e pontuacao */ 
    ASSIGN, SEMICOLON, COMMA, LPAREN, RPAREN, 
    LBRACE, RBRACE, LBRACKET, RBRACKET, 
 
    /* Palavras reservadas */ 
    MAIN, IF, ELSE, FOR, RETURN, INT, CHAR, PRINT 
} TokenType; 
 
/* Nomes dos tokens, usados apenas pelo main() de teste abaixo para 
 * imprimir o tipo de cada token de forma legivel. Mantenha esta lista 
 * na MESMA ORDEM do enum TokenType. */ 
static const char *nome_token[] = { 
    "UNDEF", "ID", "END_OF_FILE", 
    "INTEGERCONST", "CHARCONST", "STRINGCONST", 
    "PLUS", "MINUS", "MUL", "DIV", "MOD", 
    "EQ", "NEQ", "LT", "GT", "LEQ", "GEQ", "AND", "OR", "NOT", 
    "ASSIGN", "SEMICOLON", "COMMA", "LPAREN", "RPAREN", 
    "LBRACE", "RBRACE", "LBRACKET", "RBRACKET", 
    "MAIN", "IF", "ELSE", "FOR", "RETURN", "INT", "CHAR", "PRINT" 
}; 
 
/* Valor semantico do token corrente. */ 
typedef struct { 
    char *symbol; 
    char *error_msg; 
} YYSTYPE; 
 
YYSTYPE microc_yylval; 
 
int linha_atual = 1; 
 
TokenType ultimo_token = UNDEF;

char lexema[4000];

typedef struct {
    int tamanho;
    int posicaoOcupada;
    char **listaString;
}tabelaStrings;

tabelaStrings tabela;

void adicionaString(tabelaStrings* tab) {

    int existe = 0;
    
    if(tab->tamanho == tab->posicaoOcupada) {
        tab->tamanho = (tab->tamanho)*2;
        char **novaListaString = realloc(tab->listaString, tab->tamanho * sizeof(char *));

        if(novaListaString == NULL) {
            //lançar exceção
        }
        tab->listaString= novaListaString;
    }
    
    for(int i = 0; i < tab->posicaoOcupada; i++) {
        if(strcmp(yytext,tab->listaString[i]) == 0) {
            existe=1;
            //return true;
        }
    }
    if(existe == 0) {
        tab->listaString[tab->posicaoOcupada++]=strdup(yytext);
        //return true;
    }
}

char* retornaLexema(int indice, tabelaStrings* tab) {
    if(indice < tab->tamanho) {
        //lançar um exceção
    }
    return tab->listaString[indice];
}

static void guarda_lexema(void) {

    adicionaString(&tabela);
    microc_yylval.symbol = strdup(yytext); 
} 
 
static int pode_ser_sinal(void) { 
    if (ultimo_token == ASSIGN || 
        ultimo_token == LPAREN || 
        ultimo_token == COMMA || 
        ultimo_token == PLUS || 
        ultimo_token == MINUS || 
        ultimo_token == MUL || 
        ultimo_token == DIV || 
        ultimo_token == MOD || 
        ultimo_token == EQ || 
        ultimo_token == NEQ || 
        ultimo_token == LT || 
        ultimo_token == GT || 
        ultimo_token == LEQ || 
        ultimo_token == GEQ || 
        ultimo_token == AND || 
        ultimo_token == OR) { 
        return 1; 
    } 
 
    return 0; 
} 



%} 
 
/* ----------------------------------------------------------------------- 
 * 2. SECAO DE DEFINICOES 
 * ------------------------------------------------------------------- */ 
 
DIGIT       [0-9] 
LETRA       [a-zA-Z_] 
ALFANUM     [a-zA-Z0-9_] 
MINUS       -

%x COMMENT STRING
 
%%  
  /* ----------------------------------------------------------------------- 
   * 3. SECAO DE REGRAS 
   * --------------------------------------------------------------------- */ 
 
  /* --- Fim de arquivo ----------------------------------------------------- 
   * Tratada explicitamente (em vez de depender do retorno automatico 0 
   * do flex), pois o token UNDEF tambem vale 0 no enum TokenType -- se 
   * dependessemos do comportamento padrao, um erro lexico seria 
   * confundido com o fim do arquivo pelo main() de teste abaixo. */ 
<INITIAL><<EOF>> { return END_OF_FILE; }
 
  /* --- Espacos em branco e quebras de linha ---------------------------- */ 
\n                  { linha_atual++; } 
[ \t\r]+            { /* ignora espacos em branco */ } 
 
  /* --- Comentarios ------------------------------------------------------ 
   * Estes ja estao implementados como exemplo de uso de estados (%x) e 
   * de tratamento de erro via EOF dentro de um estado especial. */ 
"//".*              { /* comentario de linha: ignora ate o fim da linha */ } 
 
"/*"                { BEGIN(COMMENT); } 
<COMMENT>"*/"       { BEGIN(INITIAL); } 
<COMMENT>\n         { linha_atual++; } 
<COMMENT><<EOF>>    { 
                        BEGIN(INITIAL);
                        microc_yylval.error_msg = "EOF em comentario"; 
                        return UNDEF; 
                    } 
<COMMENT>.          { /* consome qualquer outro caractere dentro do comentario */ } 
 
  /* Fechamento de comentario sem abertura correspondente. */ 
"*/"                { 
                        microc_yylval.error_msg = "Comentario nao iniciado"; 
                        return UNDEF; 
                    } 
 
{LETRA}{ALFANUM}*   { 
    if (strcmp(yytext, "if")==0){ 
        ultimo_token = IF; 
        return IF; 
    } 

    if (strcmp(yytext, "main")==0){ 
        ultimo_token = MAIN; 
        return MAIN; 
    } 
 
    if (strcmp(yytext, "int")==0){ 
        ultimo_token = INT; 
        return INT; 
    } 
 
    if (strcmp(yytext, "else")==0){ 
        ultimo_token = ELSE; 
        return ELSE; 
    } 
 
    if (strcmp(yytext, "for")==0){ 
        ultimo_token = FOR; 
        return FOR; 
    } 
 
    if (strcmp(yytext, "return")==0){ 
        ultimo_token = RETURN; 
        return RETURN; 
    } 
 
    if (strcmp(yytext, "char")==0){ 
        ultimo_token = CHAR; 
        return CHAR; 
    } 
 
    if (strcmp(yytext, "print")==0){ 
        ultimo_token = PRINT; 
        return PRINT; 
    } 
 
    guarda_lexema(); 
    ultimo_token = ID; 
    return ID; 
} 

 
{MINUS}{DIGIT}+ { 
    if (pode_ser_sinal()) { 
        guarda_lexema(); 
        ultimo_token = INTEGERCONST; 
        return INTEGERCONST; 
    } else { 
        yyless(1);  /* faz com que o flex devolva o que foi consumido de maneira "errada", nesse caso um numero colado ao sinal de - por exemplo, e trate-os separadamente partindo do primeiro digito */ 
        ultimo_token = MINUS; 
        return MINUS; 
    } 
} 
 
{DIGIT}+            { 
                        guarda_lexema(); 
                        ultimo_token = INTEGERCONST; 
                        return INTEGERCONST; 
                    } 
 

'[^'\n]' { 
    guarda_lexema(); 
    ultimo_token = CHARCONST; 
    return CHARCONST; 
} 
 
'[^'\n]* { 
    microc_yylval.error_msg = "Char constante nao terminada";
    return UNDEF; 
} 
 



\" {
    int caractere;
    int posicao = 0;
    int erro = 0;

    while (1){
        caractere = input();

        if (caractere == '"'){
            lexema[posicao] = '\0';
            microc_yylval.symbol = strdup(lexema);
            return STRINGCONST;
        }
        else if (caractere == EOF){
            erro = 1;
            break;
        }
        else if (caractere == '\n'){
            erro = 2;
            break;
        }
        else if (caractere == '\0'){
            erro = 3;
            break;
        }
        else if (caractere == '\\'){
            caractere = input();

            if (caractere == 'n'){
                lexema[posicao] = '\n';
                posicao++;
            }
            else if (caractere == 't'){
                lexema[posicao] = '\t';
                posicao++;
            }
            else if (caractere == '\\'){
                lexema[posicao] = '\\';
                posicao++;
            }
            else if (caractere == '"'){
                lexema[posicao] = '"';
                posicao++;
            }
            else if (caractere == '0'){
                lexema[posicao] = '\0';
                posicao++;
            }
            else{
                lexema[posicao] = '\\';
                posicao++;
                lexema[posicao] = caractere;
                posicao++;
            }
        }
        else{
            lexema[posicao] = caractere;
            posicao++;
        }
    }

    if (erro == 1){
        microc_yylval.error_msg = "EOF em string";
        return UNDEF;
    }
    else if (erro == 2){
        microc_yylval.error_msg = "String nao terminada";
        return UNDEF;
    }
    else if (erro == 3){
        microc_yylval.error_msg = "String contem caractere nulo";
        return UNDEF;
    }
}
 
 
"=="                { ultimo_token = EQ; return EQ; } 
"="                 { ultimo_token = ASSIGN; return ASSIGN; } 
 
"!="                { ultimo_token = NEQ; return NEQ; } 
"!"                 { ultimo_token = NOT; return NOT; } 
 
"<="                { ultimo_token = LEQ; return LEQ; } 
"<"                 { ultimo_token = LT; return LT; } 
 
">="                { ultimo_token = GEQ; return GEQ; } 
">"                 { ultimo_token = GT; return GT; } 
 
"&&"                { ultimo_token = AND; return AND; } 
 
"||"                { ultimo_token = OR; return OR; } 
 
 
  /* --- Operadores aritmeticos e simbolos de pontuacao (ja prontos) ------ */ 
"+"                 { ultimo_token = PLUS; return PLUS; } 
 
"-"                 {  
                        ultimo_token = MINUS; 
                        return MINUS;  
                    } 
 
"*"                 { ultimo_token = MUL; return MUL; } 
"/"                 { ultimo_token = DIV; return DIV; } 
"%"                 { ultimo_token = MOD; return MOD; } 
";"                 { ultimo_token = SEMICOLON; return SEMICOLON; } 
","                 { ultimo_token = COMMA; return COMMA; } 
"("                 { ultimo_token = LPAREN; return LPAREN; } 
")"                 { ultimo_token = RPAREN; return RPAREN; } 
"{"                 { ultimo_token = LBRACE; return LBRACE; } 
"}"                 { ultimo_token = RBRACE; return RBRACE; } 
"["                 { ultimo_token = LBRACKET; return LBRACKET; } 
"]"                 { ultimo_token = RBRACKET; return RBRACKET; } 
 


.                   { 
                        microc_yylval.error_msg = strdup(yytext); 
                        return UNDEF; 
                    } 
 
%% 
 
/* ----------------------------------------------------------------------- 
 * 4. SUB-ROTINAS DO USUARIO 
 * ------------------------------------------------------------------- */ 

int yywrap(void) { 
    return 1; 
} 
 
int main(int argc, char **argv) { 
    tabela.tamanho=100;
    tabela.posicaoOcupada=0;
    tabela.listaString= (char**)malloc(tabela.tamanho *sizeof(char *));




    if (argc < 2) { 
        fprintf(stderr, "Uso: %s <arquivo.mc>\n", argv[0]); 
        return 1; 
    } 
 
    FILE *arquivo_fonte = fopen(argv[1], "r"); 
    if (!arquivo_fonte) { 
        fprintf(stderr, "Erro: nao foi possivel abrir o arquivo '%s'\n", argv[1]); 
        return 1; 
    } 
 
    yyin = arquivo_fonte; 
 
    int tipo; 
    while ((tipo = yylex()) != END_OF_FILE) { 
        if (tipo == UNDEF) { 
            fprintf(stderr, "ERRO LEXICO (linha %d): %s\n", 
                    linha_atual, microc_yylval.error_msg); 
            continue; 
        } 
 

        if (tipo == STRINGCONST || tipo == ID || tipo == INTEGERCONST || tipo == CHARCONST){
            printf("Token: tipo = %-13s lexema = '%s'  linha = %d\n", nome_token[tipo], microc_yylval.symbol, linha_atual);
        }
        else{
            printf("Token: tipo = %-13s lexema = '%s'  linha = %d\n", nome_token[tipo], yytext, linha_atual);
        }


    } 
    fclose(arquivo_fonte); 
    return 0; 
}