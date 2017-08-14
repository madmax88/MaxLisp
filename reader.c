#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lisp.h"
#include "reader.h"

extern int yylex();
extern FILE *yyin;
extern char *yytext;

extern lisp_object_t *NIL;

static lisp_object_t *read_string();

lisp_object_t *read_object(FILE *input) {
  yyin = input;

  enum token next_token = yylex();

  switch(next_token) {

  case TOKEN_QUOTE:
    printf("pre-func\n");
    return read_string();

  default:
    break;
  }

  return NIL;
}

static lisp_object_t *read_string() {
  char *target_str = malloc(strlen(yytext));
  char *target_ptr = target_str;
  char *ptr = yytext;

  lisp_object_t *string_object = make_lisp_object();
  string_object->type = STRING;

  while (*ptr && *ptr != '"') {
    if (*ptr == '\\' && *(ptr + 1) == '"') {
      ptr+=2;
      *target_ptr = '"';
      target_ptr++;
      continue;
    } 

    *target_ptr = *ptr;
    target_ptr++;
    ptr++;
  }
  *target_ptr = 0;

  string_object->datum.string = target_str;

  return string_object;
}
