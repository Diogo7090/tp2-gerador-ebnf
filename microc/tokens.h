#ifndef TOKENS_H
#define TOKENS_H

#include <stdio.h>

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
 
typedef struct {
    TokenType tipo;
    char lexema[100];
    int linha;
} Token;

#endif