#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lisp.h"
#include "reader.h"

enum read_condition read_flag = 0;

extern int yylex();

extern FILE *yyin;
extern char *yytext;
extern int yyleng;

extern void my_savior(int c, char *yytexptr);
extern void switch_buffer(FILE *file);

extern lisp_object_t *NIL;
extern lisp_object_t *T;

static lisp_object_t *read_string();
static lisp_object_t *read_double();
static lisp_object_t *read_symbol();
static lisp_object_t *read_cons(FILE *file);
static lisp_object_t *read_quoted(FILE *file);
static lisp_object_t *read_backquote(FILE *file);
static lisp_object_t *read_comma(FILE *file);
static lisp_object_t *read_comma_at(FILE *file);

static void clear_rest_of_list(int num_open_parens);

lisp_object_t* read_object(FILE *input, char *prompt) {
  static FILE *in = NULL;
  read_flag = 0;

  if (in != input) {
    switch_buffer(input);
    in = input;
  }

  if (prompt != NULL)
    printf("%s", prompt);

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
    read_flag = READ_EOF;
    return NULL;

  case TOKEN_RIGHT_PAREN:
    read_flag = READ_UNBALANCED_PAREN;
    return NULL;

  case TOKEN_SINGLE_QUOTE:
    return read_quoted(input);

  case TOKEN_DOT:
    read_flag = READ_DOT;
    return NULL;

  case TOKEN_BACKQUOTE:
    return read_backquote(input);

  case TOKEN_COMMA:
    return read_comma(input);

  case TOKEN_COMMA_AT:
    return read_comma_at(input);

  default:
    break;
  }

  return NIL;
}

static lisp_object_t* read_quoted(FILE *file) {
  lisp_object_t *quote_symbol = make_lisp_object();
  quote_symbol->type = SYMBOL;
  quote_symbol->datum.symbol = strdup("quote");

  lisp_object_t *quote_object = read_object(file, NULL);

  return make_cons(quote_symbol, make_cons(quote_object, NIL));
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
    } else if (*ptr == '\\' && *(ptr + 1) == 'n') {
      ptr += 2;
      *target_ptr = '\n';
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

  /* we have a wasteful alloc going on here, but it works (for now) */
  if (!strcmp(symbol_contents, "nil"))
    return NIL;

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

    lisp_object_t *next_object = read_object(file, NULL);

    /* handles improper lists */
    if (read_flag == READ_DOT) {
      if (CONS_VALUE(cons_o)->car == NULL) {
        fprintf(stderr, "Error: invalid usage of '.'.\n");
        return NULL;
      }

      next_object = read_object(file, NULL);
      if (next_object == NULL) {
        return NULL;            /* some other help message should be printed */
      }
      *next_cons_ref = next_object;
      
      next_token = yylex();

      /* make sure the last element of the list is actually the last! */
      if (next_token != TOKEN_RIGHT_PAREN && next_token != TOKEN_EOF) {
        clear_rest_of_list(num_parens);
        fprintf(stderr, "Error: invalid usage of '.'.\n");
        return NULL;
      } else if (next_token == TOKEN_EOF) {
        fprintf(stderr, "Error: unexpected EOF.\n");
        read_flag = READ_EOF;
        return NULL;
      }

      return cons_o;
    }

    if (!next_object) {
      fprintf(stderr, "Error: read_object() returned an unexpected NULL pointer.\n");
      return NULL;
    }

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

static void clear_rest_of_list(int num_open_parens) {
  int num_parens = num_open_parens;

  while (num_parens) {
    enum token next_token = yylex();

    if (next_token == TOKEN_LEFT_PAREN)
      ++num_parens;
    else if (next_token == TOKEN_RIGHT_PAREN)
      --num_parens;
  }
}

static lisp_object_t *read_backquote(FILE *file) {
  lisp_object_t *backquote = make_lisp_object();
  lisp_object_t *backquoted_object = NULL;
  lisp_object_t *next_object = NULL;
  
  backquote->type = SYMBOL;
  backquote->datum.symbol = strdup("backquote");

  next_object = read_object(file, NULL);

  if (next_object != NULL) {
    backquoted_object = make_cons(backquote,
                                  make_cons(next_object, NIL));
  }

  return backquoted_object;
}

static lisp_object_t *read_comma(FILE *file) {
  lisp_object_t *comma = make_lisp_object();
  lisp_object_t *comma_object = NULL;
  lisp_object_t *next_object = NULL;
  
  comma->type = SYMBOL;
  comma->datum.symbol = strdup("comma");

  next_object = read_object(file, NULL);

  if (next_object != NULL) {
    comma_object = make_cons(comma,
                          make_cons(next_object, NIL));
  }

  return comma_object;
}

static lisp_object_t *read_comma_at(FILE *file) {
  lisp_object_t *comma_at = make_lisp_object();
  lisp_object_t *comma_at_object = NULL;
  lisp_object_t *next_object = NULL;
  
  comma_at->type = SYMBOL;
  comma_at->datum.symbol = strdup("comma-at");

  next_object = read_object(file, NULL);

  if (next_object != NULL) {
    comma_at_object = make_cons(comma_at,
                             make_cons(next_object, NIL));
  }

  return comma_at_object;
}
