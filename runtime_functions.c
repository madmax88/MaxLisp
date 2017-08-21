#include <stdio.h>
#include <stdlib.h>
#include "lisp.h"
#include "runtime_functions.h"

extern lisp_object_t *NIL;

lisp_object_t* cons_func(lisp_object_t *args) {
  if (args == NIL) {
    fprintf(stderr, "Error: cons requires 2 arguments, but received 0.\n");
    return NULL;
  } else if (CONS_VALUE(args)->cdr == NIL) {
    fprintf(stderr, "Error: cons requires 2 arguments, but received only 1.\n");
    return NULL;
  } else if (CONS_VALUE(args)->cdr->type != CONS) {
    fprintf(stderr, "Error: cons requires a proper list.\n");
    return NULL;
  }

  lisp_object_t *car = CONS_VALUE(args)->car;
  lisp_object_t *cadr = CONS_VALUE(CONS_VALUE(args)->cdr)->car;

  return make_cons(car, cadr);
}

lisp_object_t* car_func(lisp_object_t *args) {
  if (args == NIL) {
    fprintf(stderr, "Error: car requires 1 argument, but received 0.\n");
    return NULL;
  }

  lisp_object_t *car = CONS_VALUE(args)->car;

  if (car->type != CONS) {
    fprintf(stderr, "Error: car defined on CONS.\n");
    return NULL;
  }

  if (car == NIL)
    return NIL;

  return CONS_VALUE(car)->car;
}

lisp_object_t* cdr_func(lisp_object_t *args) {
  if (args == NIL) {
    fprintf(stderr, "Error: cdr requires 1 argument, but received 0.\n");
    return NULL;
  }

  lisp_object_t *car = CONS_VALUE(args)->car;

  if (car->type != CONS) {
    fprintf(stderr, "Error: cdr defined on CONS.\n");
    return NULL;
  }

  if (car == NIL)
    return NIL;

  return CONS_VALUE(car)->cdr;
}
