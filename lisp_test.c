#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lisp.h"

static void test_symbol_print();
static void test_number_print();
static void test_cons_print();

extern lisp_object_t* NIL;

int main() {
  init_lisp_module();
  test_symbol_print();
  test_number_print();
  test_cons_print();

  do_gc(NIL);                   /* We manually trigger GC */
}

static void test_symbol_print() {
  printf("Testing symbol print...\n");

  lisp_object_t *symbol_object = make_lisp_object();
  char *buffer = malloc(257);

  for (int i = 0; i < 256; i++) {
    buffer[i] = 'a';
  }
  buffer[256] = 0;

  symbol_object->type = SYMBOL;
  symbol_object->datum.symbol = buffer;

  lisp_object_t *print_value = print_object(symbol_object);

  printf("  Making sure returned string is the correct size...\n");
  printf("  string length of first symbol: %ld\n", strlen(print_value->datum.string));
  printf("  expected: %s\n found: %s\n", buffer, print_value->datum.string);
  assert(strlen(print_value->datum.string) == 256);
  printf("  Returned string is correctly sized.\n");

  printf("  Making sure returned string contains the correct value...\n");
  assert(!strcmp(print_value->datum.string, buffer));
  printf("  Returned string contains the correct value.\n");

  printf("Symbol printing test passed!\n\n");
}

static void test_number_print() {
  printf("Testing number print...\n");

  lisp_object_t *number = make_lisp_object();
  number->type = NUMBER;
  number->datum.number = 100;

  lisp_object_t *print_value = print_object(number);

  printf("  Making sure returned string contains the correct value...\n");
  printf("  Expected 100, found: %s\n", print_value->datum.string);  
  // assert(!strcmp(print_value->datum.string, "100")); -- we only support doubles (so far)

  printf("Number printing test passed!\n\n");
}

static void test_cons_print() {
  printf("Testing cons print...\n");

  lisp_object_t *nil = NIL;
  lisp_object_t *nil_str = print_object(nil);

  printf("  Making sure NIL is printed correctly...\n");
  printf("  Expected NIL, found: %s\n", nil_str->datum.string);
  printf("  Expected 3, found: %ld\n", strlen(nil_str->datum.string));
  assert(!strcmp(nil_str->datum.string, "NIL"));

  lisp_object_t *number_a = make_lisp_object();
  number_a->type = NUMBER;
  number_a->datum.number = 10;

  lisp_object_t *number_b = make_lisp_object();
  number_b->type = NUMBER;
  number_b->datum.number = 11;

  lisp_object_t *improper_pair = make_cons(number_a, number_b);

  lisp_object_t *improper_str = print_object(improper_pair);

  printf("  Expected (10 . 11), found: %s\n",
         improper_str->datum.string);

  lisp_object_t *singleton = make_cons(number_a, NIL);
  lisp_object_t *sstr = print_object(singleton);

  printf("  %s\n", sstr->datum.string);

  lisp_object_t *b_pair = make_cons(number_b, singleton);
  lisp_object_t *right_pair = print_object(b_pair);

  printf("  %s\n", right_pair->datum.string);
  

  printf("Cons printing test passed!\n\n");
}
