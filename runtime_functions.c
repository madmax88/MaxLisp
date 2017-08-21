#include <stdio.h>
#include <stdlib.h>
#include "lisp.h"
#include "runtime_functions.h"

extern lisp_object_t *NIL;

/* TODO: add checks to make sure that the actual length of these arg 
   lists is actually correct. 
   Currently we ignore unnecessary arguments (which is bad.)
*/

lisp_object_t* quote_func(lisp_object_t *args) {
  if (args == NIL) {
    fprintf(stderr, "Error: quote requires 1 argument, but received 0.\n");
    return NULL;
  }

  return CONS_VALUE(args)->car;
}

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

lisp_object_t* set_func(lisp_object_t *expression, lisp_object_t *environment) {
  if (CONS_VALUE(expression)->cdr == NIL) {
    fprintf(stderr, "Error: set requires 2 arguments, but received 0.\n");
    return NULL;
  } else if (CONS_VALUE(CONS_VALUE(expression)->cdr)->cdr == NIL) {
    fprintf(stderr, "Error: set requires 2 arguments, but received 1.\n");
    return NULL;
  }
  lisp_object_t *symbol_value = eval(CONS_VALUE(CONS_VALUE(expression)->cdr)->car,
                                     environment);

  if (!symbol_value)
    return NULL;
      
  lisp_object_t *bind_value = eval(CONS_VALUE(CONS_VALUE(CONS_VALUE(expression)->cdr)->cdr)->car,
                                   environment);

  if (!bind_value)
    return NULL;

  set(symbol_value, bind_value, environment);
  return NIL;
}

lisp_object_t* if_func(lisp_object_t *args, lisp_object_t *environment) {
  if (args == NIL) {
    fprintf(stderr, "Error: if requires at least 2 arguments, but received 0.\n");
    return NULL;
  }
  
  lisp_object_t *antecedent = CONS_VALUE(args)->car;
  
  if (CONS_VALUE(args)->cdr == NIL) {
    fprintf(stderr, "Error: if requires at least 2 arguments, but received 1.\n");
    return NULL;
  } else if (CONS_VALUE(args)->cdr->type != CONS) {
    fprintf(stderr, "Error: if syntax.\n");
    return NULL;
  }

  lisp_object_t *consequent = CONS_VALUE(CONS_VALUE(args)->cdr)->car;

  lisp_object_t *otherwise = NIL;
  if (CONS_VALUE(CONS_VALUE(args)->cdr)->cdr != NIL) {
    if (CONS_VALUE(CONS_VALUE(args)->cdr)->cdr->type == CONS) {
      otherwise = CONS_VALUE(CONS_VALUE(CONS_VALUE(args)->cdr)->cdr)->car;
    } else {
      fprintf(stderr, "Error: if syntax.\n");
      return NULL;
    }
  }

  lisp_object_t *result = eval(antecedent, environment);

  if (!result)
    return NULL;

  if (result != NIL) {
    return eval(consequent, environment);
  } else {
    return eval(otherwise, environment);
  }
}
