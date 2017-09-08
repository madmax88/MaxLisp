#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lisp.h"
#include "reader.h"
#include "runtime_functions.h"

extern lisp_object_t *NIL;
extern lisp_object_t *T;

/* TODO: add checks to make sure that the actual length of these arg 
   lists is actually correct. 
   Currently we ignore unnecessary arguments (which is bad.)
*/

static int arg_length(lisp_object_t *args) {
  lisp_object_t *arglen = length(make_cons(args, NIL));

  /* handles errors */
  if (arglen == NULL || arglen == NIL)
    return 0;

  return (int) (arglen->datum.number);
}

lisp_object_t* quote_func(lisp_object_t *args) {
  int num_args = arg_length(args);

  if (num_args != 1) {
    fprintf(stderr, "Error: quote requires 1 arguments.\n");
    return NIL;
  }

  return CONS_VALUE(args)->car;
}

lisp_object_t* cons_func(lisp_object_t *args) {
  int num_args = arg_length(args);

  if (num_args != 2) {
    fprintf(stderr, "Error: cons requires 2 arguments.\n");
    return NIL;
  }

  lisp_object_t *car = CONS_VALUE(args)->car;
  lisp_object_t *cadr = CONS_VALUE(CONS_VALUE(args)->cdr)->car;

  return make_cons(car, cadr);
}

lisp_object_t* car_func(lisp_object_t *args) {
  int num_args = arg_length(args);

  if (num_args != 1) {
    fprintf(stderr, "Error: car requires 1 arguments.\n");
    return NIL;
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
  int num_args = arg_length(args);

  if (num_args != 1) {
    fprintf(stderr, "Error: cdr requires 1 arguments.\n");
    return NIL;
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
  int num_args = arg_length(args);

  if (num_args < 2) {
    fprintf(stderr, "Error: if requires at least 2 arguments.\n");
    return NIL;
  } else if (num_args > 3) {
    fprintf(stderr, "Error: if accepts no more than 3 arguments.\n");
    return NIL;
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

lisp_object_t* list(lisp_object_t *args) {
  lisp_object_t *the_list = make_cons(NULL, NULL);
  lisp_object_t **prev_ref = &the_list;

  lisp_object_t *lb = the_list;
  lisp_object_t *it = args;

  while (it != NIL) {
    CONS_VALUE(lb)->car = CONS_VALUE(it)->car;

    CONS_VALUE(lb)->cdr = make_cons(NULL, NULL);
    prev_ref = &(CONS_VALUE(lb)->cdr);

    lb = CONS_VALUE(lb)->cdr;
    it = CONS_VALUE(it)->cdr;
  }
  *prev_ref = NIL;

  return the_list;
}

lisp_object_t* length(lisp_object_t *args) {
  if (args == NIL) {
    fprintf(stderr, "Error: length requires 1 argument, but was supplied 0.\n");
    return NIL;
  } else if (CONS_VALUE(args)->cdr != NIL) {
    fprintf(stderr, "Error: length supplied too many arguments.\n");
    return NIL;
  }

  size_t length = 0;

  lisp_object_t *t = CONS_VALUE(args)->car;
  while (t != NIL && t->type == CONS) {
    length++;

    t = CONS_VALUE(t)->cdr;
  }

  if (t->type != CONS)
    length++;

  lisp_object_t *length_o = make_lisp_object();
  length_o->type = NUMBER;
  length_o->datum.number = length;
  
  return length_o;
}

lisp_object_t* eq(lisp_object_t *args) {
  int num_args = arg_length(args);

  if (num_args != 2) {
    fprintf(stderr, "Error: eq requires 2 arguments.\n");
    return NIL;
  }

  lisp_object_t *a = CONS_VALUE(args)->car;
  lisp_object_t *b = CONS_VALUE(CONS_VALUE(args)->cdr)->car;

  if (a->type != b->type)
    return NIL;

  switch (a->type) {

  case NUMBER:
    return (a->datum.number == b->datum.number) ? T : NIL;

  case STRING:
    return strcmp(a->datum.string, b->datum.string) == 0 ? T : NIL;

  case SYMBOL:
    return strcmp(a->datum.symbol, b->datum.symbol) == 0 ? T : NIL;

  case NATIVE_FUNCTION:
    return (a == b) ? T : NIL;

  case CONS:
    return (a == b) ? T : NIL;

  case LAMBDA:
    return (a == b) ? T : NIL;

  case MACRO:
    return (a == b) ? T : NIL;

  default:
    fprintf(stderr, "Error: eq is not defined on type.\n");
    return NIL;
  }
}

lisp_object_t* load(lisp_object_t *args, lisp_object_t *env) {
  int num_args = arg_length(args);

  if (num_args != 1) {
    fprintf(stderr, "Error: load requires 1 argument.\n");
    return NIL;
  } 

  lisp_object_t *path = CONS_VALUE(args)->car;

  if (path->type != STRING) {
    fprintf(stderr, "Error: load expects its first argument to be of type STRING.\n");
    return NULL;
  }

  FILE *file = fopen(path->datum.string, "r");

  if (!file) {
    fprintf(stderr, "Error: load unable to open file: \"%s\"\n", path->datum.string);
    return NULL;
  }

  lisp_object_t *next_object = NULL;
  while ((next_object = read_object(file, NULL)) != NULL) {
    eval(next_object, env);
  }

  fclose(file);

  return T;
}

lisp_object_t* atomp(lisp_object_t *args) {
  int num_args = arg_length(args);

  if (num_args != 1) {
    fprintf(stderr, "Error: atom? requires 1 argument.\n");
    return NIL;
  } 

  lisp_object_t *len_lst = make_cons(args, NIL);
  lisp_object_t *len = length(len_lst);

  if (len != NIL && len->datum.number != 1) {
    fprintf(stderr, "Error: atom? requires 1 argument.\n");
    return NIL;
  }

  lisp_object_t *a = CONS_VALUE(args)->car;

  if (a->type != CONS || a == NIL)
    return T;

  return NIL;
}

lisp_object_t* primitive_print(lisp_object_t *args) {
  lisp_object_t *str = NULL;

  int num_args = arg_length(args);

  if (num_args != 1) {
    fprintf(stderr, "Error: primitive-print requires 1 argument.\n");
    return NIL;
  } else {
    if (CONS_VALUE(args)->car->type == STRING) {
      str = CONS_VALUE(args)->car;
    } else {
      str = print_object(CONS_VALUE(args)->car);
    }
  }

  if (str)
    printf("%s ", str->datum.string);

  return NIL;
}

char* strdup(const char *src) {
  size_t len = strlen(src);
  char *copy = malloc(len + 1);

  strcpy(copy, src);
  
  return copy;
}

lisp_object_t* add(lisp_object_t *args) {
  int num_args = arg_length(args);

  if (num_args < 1) {
    fprintf(stderr, "Error: too few arguments supplied to +.\n");
    return NULL;
  }

  lisp_object_t *it = args;
  double sum = 0;
  
  while (it != NIL) {
    if (CONS_VALUE(it)->car->type != NUMBER) {
      fprintf(stderr, "Error: wrong non-numeric type given to +.\n");
      return NULL;
    }

    sum += CONS_VALUE(it)->car->datum.number;
    it = CONS_VALUE(it)->cdr;
  }

  lisp_object_t *result = make_lisp_object();
  result->type = NUMBER;
  result->datum.number = sum;

  return result;
}

lisp_object_t* subtract(lisp_object_t *args) {
  int num_args = arg_length(args);

  if (num_args < 1) {
    fprintf(stderr, "Error: too few arguments supplied to -\n");
    return NULL;
  }

  if (CONS_VALUE(args)->car->type != NUMBER) {
    fprintf(stderr, "Error: wrong non-numeric type given to -.\n");
    return NULL;
  }

  double value = CONS_VALUE(args)->car->datum.number;
  lisp_object_t *it = CONS_VALUE(args)->cdr;
  
  while (it != NIL) {
    if (CONS_VALUE(it)->car->type != NUMBER) {
      fprintf(stderr, "Error: wrong non-numeric type given to -.\n");
      return NULL;
    }

    value -= CONS_VALUE(it)->car->datum.number;
    it = CONS_VALUE(it)->cdr;
  }

  lisp_object_t *result = make_lisp_object();
  result->type = NUMBER;
  result->datum.number = value;

  return result;
}

lisp_object_t* multiply(lisp_object_t *args) {
  int num_args = arg_length(args);

  if (num_args < 1) {
    fprintf(stderr, "Error: too few arguments supplied to *.\n");
    return NULL;
  }

  lisp_object_t *it = args;
  double product = 1;
  
  while (it != NIL) {
    if (CONS_VALUE(it)->car->type != NUMBER) {
      fprintf(stderr, "Error: wrong non-numeric type given to *.\n");
      return NULL;
    }

    product *= CONS_VALUE(it)->car->datum.number;
    it = CONS_VALUE(it)->cdr;
  }

  lisp_object_t *result = make_lisp_object();
  result->type = NUMBER;
  result->datum.number = product;

  return result;
}

lisp_object_t* divide(lisp_object_t *args) {
  int num_args = arg_length(args);

  if (num_args < 1) {
    fprintf(stderr, "Error: too few arguments supplied to /.\n");
    return NULL;
  }

  if (CONS_VALUE(args)->car->type != NUMBER) {
    fprintf(stderr, "Error: wrong non-numeric type given to /.\n");
    return NULL;
  }

  double value = CONS_VALUE(args)->car->datum.number;
  lisp_object_t *it = CONS_VALUE(args)->cdr;
  
  while (it != NIL) {
    if (CONS_VALUE(it)->car->type != NUMBER) {
      fprintf(stderr, "Error: wrong non-numeric type given to /.\n");
      return NULL;
    }

    value /= CONS_VALUE(it)->car->datum.number;
    it = CONS_VALUE(it)->cdr;
  }

  lisp_object_t *result = make_lisp_object();
  result->type = NUMBER;
  result->datum.number = value;

  return result;
}

lisp_object_t* less_than(lisp_object_t *args) {
  int num_args = arg_length(args);

  if (num_args < 1) {
    fprintf(stderr, "Error: too few arguments supplied to <.\n");
    return NULL;
  }
  
  if (CONS_VALUE(args)->car->type != NUMBER) {
    fprintf(stderr, "Error: wrong non-numeric type given to <.\n");
    return NULL;
  }

  lisp_object_t *it = CONS_VALUE(args)->cdr;
  lisp_object_t *prev = CONS_VALUE(args)->car;

  while (it != NIL) {
    if (CONS_VALUE(it)->car->type != NUMBER) {
      fprintf(stderr, "Error: wrong non-numeric type given to <.\n");
      return NULL;
    }

    if (CONS_VALUE(it)->car->datum.number <= prev->datum.number)
      return NIL;

    prev = CONS_VALUE(it)->car;
    it = CONS_VALUE(it)->cdr;
  }

  return T;
}

lisp_object_t* greater_than(lisp_object_t *args) {
  int num_args = arg_length(args);

  if (num_args < 1) {
    fprintf(stderr, "Error: too few arguments supplied to >.\n");
    return NULL;
  }
  
  if (CONS_VALUE(args)->car->type != NUMBER) {
    fprintf(stderr, "Error: wrong non-numeric type given to >.\n");
    return NULL;
  }

  lisp_object_t *it = CONS_VALUE(args)->cdr;
  lisp_object_t *prev = CONS_VALUE(args)->car;

  while (it != NIL) {
    if (CONS_VALUE(it)->car->type != NUMBER) {
      fprintf(stderr, "Error: wrong non-numeric type given to >.\n");
      return NULL;
    }

    if (CONS_VALUE(it)->car->datum.number >= prev->datum.number)
      return NIL;

    prev = CONS_VALUE(it)->car;
    it = CONS_VALUE(it)->cdr;
  }

  return T;
}
