#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lisp.h"
#include "reader.h"

extern int yylex();

extern FILE *yyin;
extern char *yytext;
extern int yyleng;
extern void my_savior(int c, char *yytexptr);

extern lisp_object_t *NIL;

static lisp_object_t *read_string();
static lisp_object_t *read_double();
static lisp_object_t *read_symbol();
static lisp_object_t *read_cons(FILE *file);

lisp_object_t* read_object(FILE *input) {
  yyin = input;

  enum token next_token = yylex();

  switch(next_token) {

  case TOKEN_QUOTE:
    return read_string();

  case TOKEN_NUMBER:
    return read_double();

  case TOKEN_SYMBOL:
    return read_symbol();

  case TOKEN_LEFT_PAREN:
    return read_cons(input);

  case TOKEN_EOF:
    return NULL;

  default:
    break;
  }

  return NIL;
}

static lisp_object_t* read_string() {
  lisp_object_t *string = make_lisp_object();
  char *target_str = malloc(yyleng);
  char *target_ptr = target_str;
  char *ptr = yytext;

  ptr++;

  while (*ptr) {
    if (*ptr == '\\' && *(ptr + 1) == '"') {
      ptr += 2;
      *target_ptr = '"';
      target_ptr++;
      continue;
    } else if (*ptr == '"') {
      break;
    }

    *target_ptr = *ptr;
    target_ptr++;
    ptr++;
  }
  *target_ptr = 0;

  string->type = STRING;
  string->datum.string = target_str;

  return string;
}

static lisp_object_t* read_double() {
  lisp_object_t *double_o = make_lisp_object();

  double_o->type = NUMBER;
  double_o->datum.number = atof(yytext);

  return double_o;
}

static lisp_object_t* read_symbol() {
  lisp_object_t *symbol_o = make_lisp_object();
  char *symbol_contents = malloc(yyleng + 1);

  symbol_o->type = SYMBOL;
  strncpy(symbol_contents, yytext, yyleng);
  symbol_contents[yyleng] = 0;

  symbol_o->datum.symbol = symbol_contents;

  return symbol_o;
}

static lisp_object_t* read_cons(FILE *file) {
  lisp_object_t *cons_o = make_cons(NULL, NULL); /* we can clean this up, maybe. */
  lisp_object_t *next_cons = cons_o;
  lisp_object_t **next_cons_ref = &cons_o;

  int num_parens = 1;

  while (num_parens) {
    enum token next_token = yylex();

    if (next_token == TOKEN_RIGHT_PAREN) {
      --num_parens;
      continue;
    } else if (next_token == TOKEN_EOF) {
      fprintf(stderr, "Error: encountered unexpected EOF.\n");
      exit(1);
    }

    char *yycopy = strdup(yytext);
    for (int i = yyleng - 1; i >= 0; --i) {
      my_savior(yycopy[i], yytext);
    }
    free(yycopy);

    lisp_object_t *next_object = read_object(file);
    ((cons*) next_cons->datum.cons)->car = next_object;
    ((cons*) next_cons->datum.cons)->cdr = make_cons(NULL, NULL);

    next_cons_ref = &(((cons*) next_cons->datum.cons)->cdr);
    next_cons = ((cons*) next_cons->datum.cons)->cdr;
  }

  if (((cons*) next_cons->datum.cons)->car == NULL &&
      ((cons*) next_cons->datum.cons)->cdr == NULL)
    *next_cons_ref = NIL;

  return cons_o;
}
