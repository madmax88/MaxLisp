#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lisp.h"
#include "runtime_functions.h"

/* TODO: we need macros!
 * 
 * Macros are fundamentally a specialized procedure.
 * We add an additional type called `MACRO`
 * that is handled by applying the `car` of the expression to the unevaluated `cdr`
 * and evaluating the result. 
 */

static reference_list_t *references = NULL;

lisp_object_t* NIL = NULL;
lisp_object_t* T   = NULL;

static void unmark_all_references();
static void mark(lisp_object_t *object);

static lisp_object_t* eval_arg_list(lisp_object_t *arg_list, lisp_object_t *env);

lisp_object_t* init_lisp_module() {
  NIL = make_cons(NULL, NULL);

  lisp_object_t *global_environment = NIL;

  T = make_lisp_object();
  T->type = SYMBOL;
  T->datum.symbol = strdup("t");
  
  global_environment = nice_set("nil", NIL, global_environment);
  nice_set("t", T, global_environment);

  /* these functions are defined in runtime_functions.h */
  register_function("cons", cons_func, global_environment);
  register_function("car",  car_func,  global_environment);
  register_function("cdr",  cdr_func,  global_environment);

  return global_environment;
}

static reference_list_t* make_reference_list(lisp_object_t *object) {
  reference_list_t* ref = malloc(sizeof(reference_list_t));
  ref->next = NULL;
  ref->node = object;
  return ref;
}

static void create_reference(lisp_object_t *object) {
  if (!references) {
    references = make_reference_list(object);
    return;
  }

  reference_list_t *current_reference = references;
  while (current_reference->next)
    current_reference = current_reference->next;

  current_reference->next = make_reference_list(object);
}

lisp_object_t* make_lisp_object() {
  lisp_object_t *object = malloc(sizeof(lisp_object_t));
  create_reference(object);

  object->marked = 0;

  return object;
}

size_t allocated_objects() {
  size_t num = 0;

  reference_list_t *current_reference = references;
  while (current_reference) {
    num++;
    current_reference = current_reference->next;
  }

  return num;
}

lisp_object_t* make_cons(lisp_object_t *car, lisp_object_t *cdr) {
  lisp_object_t *object = make_lisp_object();
  object->datum.cons = malloc(sizeof(cons));
  object->type = CONS;

  cons *object_cons = (cons*) object->datum.cons;
  object_cons->car = car;
  object_cons->cdr = cdr;

  return object;
}

lisp_object_t* deep_copy(lisp_object_t *src) {
  if (src == NULL)
    return NULL;
  else if (src == NIL)
    return NIL;
  
  lisp_object_t *dest = make_lisp_object();
  dest->type = src->type;

  cons* src_cons = NULL;
  
  switch (src->type) {

  case SYMBOL:
    strcpy(dest->datum.symbol, src->datum.symbol);
    break;

  case NUMBER:
    dest->datum.number = src->datum.number;
    break;
    
  case STRING:
    strcpy(dest->datum.string, src->datum.string);
    break;

  case CONS:                    /* WE DO NOT LIKE CIRCULARLY LINKED LISTS! */
    src_cons = (cons*) src->datum.cons;
    dest = make_cons(deep_copy(src_cons->car), deep_copy(src_cons->cdr));
    break;

  case LAMBDA:
    src_cons = (cons*) src->datum.cons;
    dest = make_cons(deep_copy(src_cons->car), deep_copy(src_cons->cdr));
    break;

  case NATIVE_FUNCTION:
    dest->datum.native_func = src->datum.native_func;
    break;

  default:
    fprintf(stderr, "ERROR: deep_copy() not defined on given type. Panicing like a coward.\n");
    exit(1);
    break;
  }

  return dest;
}

lisp_object_t* print_object(lisp_object_t *object) {
  size_t string_size = 256;
  char *dest = malloc(string_size);
  lisp_object_t *lisp_string = make_lisp_object();
  lisp_object_t *cdr;

  int strlen = 0;
  size_t temp = 0;
  memset(dest, 0, string_size);

  switch (object->type) {

  case SYMBOL:
    temp = snprintf(dest, string_size, "%s", object->datum.symbol);

    if (temp >= string_size) {
      string_size = temp + 1;
      dest = realloc(dest, string_size);
      memset(dest, 0, string_size);
      strlen = snprintf(dest, string_size, "%s", object->datum.symbol);
    } else {
      strlen = temp;
    }
    break;

  case STRING:
    temp = snprintf(dest, string_size, "\"%s\"", object->datum.string);

    if (temp >= string_size) {
      string_size = temp + 1;
      dest = realloc(dest, string_size);
      memset(dest, 0, string_size);
      strlen = snprintf(dest, string_size, "\"%s\"", object->datum.string);
    } else {
      strlen = temp;
    }
    break;

  case NUMBER:
    strlen = snprintf(dest, string_size, "%f", object->datum.number);

    if (strlen >= string_size) {
      dest = realloc(dest, strlen + 1);
      memset(dest, 0, strlen + 1);
      snprintf(dest, strlen + 1, "%f", object->datum.number);
    }
    break; 

  case CONS: 
    cdr = object;
    if (object == NIL) {
      strcpy(dest, "nil");
      dest[3] = 0;
      break;
    }

    temp = snprintf(dest, string_size, "(");
    if (temp >= string_size) {
      string_size += temp + 1;
      dest = realloc(dest, string_size);
      memset(dest + strlen, 0, string_size - strlen);

      temp = snprintf(dest + strlen, string_size, "(");
      strlen += temp;
    } else {
      strlen += temp;
    }

    while (cdr != NIL) {
      lisp_object_t *car = ((cons*) cdr->datum.cons)->car;
      lisp_object_t *car_str = print_object(car);

      temp = snprintf(dest + strlen, string_size - strlen, "%s", car_str->datum.string);

      if (temp >= (string_size - strlen)) {
        string_size += temp + 1;
        dest = realloc(dest, string_size);
        memset(dest + strlen, 0, string_size - strlen);

        temp = snprintf(dest + strlen, string_size - strlen, "%s", car_str->datum.string);
        strlen += temp;
      } else {
        strlen += temp;
      }

      if (((cons*) cdr->datum.cons)->cdr->type != CONS) {
        cdr = ((cons*) cdr->datum.cons)->cdr;
        lisp_object_t *cdr_str = print_object(cdr);
        temp = snprintf(dest + strlen, string_size - strlen, " . %s", cdr_str->datum.string);
        if (temp >= (string_size - strlen)) {
          string_size += temp + 1;
          dest = realloc(dest, string_size);
          memset(dest + strlen, 0, string_size - strlen);
          strlen += snprintf(dest + strlen, string_size - strlen, " . %s", cdr_str->datum.string);
        } else {
          strlen += temp;
        }

        break;
      }

      cdr = ((cons*) cdr->datum.cons)->cdr;

      if (cdr != NIL) {
        temp = snprintf(dest + strlen, string_size - strlen, " ");
        if (temp >= (string_size - strlen)) {
          string_size += temp + 1;
          dest = realloc(dest, string_size);
          memset(dest + strlen, 0, string_size - strlen);
          strlen += snprintf(dest + strlen, string_size - strlen, " ");
        } else {
          strlen += temp;
        }

      }
    }

    temp = snprintf(dest + strlen, string_size - strlen, ")");
    if (temp >= string_size) {
      string_size += temp + 1;
      dest = realloc(dest, string_size);
      memset(dest + strlen, 0, string_size - strlen);
      snprintf(dest + strlen, string_size, ")");
    }

    break;

  case NATIVE_FUNCTION:
    dest = strdup("NATIVE_FUNCTION");
    break;

  default:
    fprintf(stderr, "ERROR: print_object() not defined on given type. Panicing like a coward.\n");
    break;
  }

  lisp_string->type = STRING;
  lisp_string->datum.string = dest;

  return lisp_string;
}

/*
 * This is a simple ``mark and sweep'' algorithm.
 * We go ahead and mark each object in the environment (and related
 * children) as used.
 * 
 * Then, we go through the reference list and delete unused references.
 */
void do_gc(lisp_object_t *root) {
  unmark_all_references();
  mark(root);

  /* cleans the head of the list */
  while (references != NULL && !references->node->marked) {
    reference_list_t *next_ptr = references->next;
    delete_object(references->node);
    free(references);
    references = next_ptr;
  }

  reference_list_t *previous_reference = references;
  
  if (!references) {
    return;
  }

  reference_list_t *current_reference = references->next;
  
  /* Finally, we can iterate through the rest of the list */
  while (current_reference) {
    reference_list_t *next_reference = current_reference->next;

    if (!current_reference->node->marked) {
      delete_object(current_reference->node);
      free(current_reference);
      previous_reference->next = next_reference;
    } else {
      previous_reference = current_reference;
    }

    current_reference = next_reference;
  }
}

void delete_object(lisp_object_t *object) {
  switch (object->type) {

  case STRING:
    if (object->datum.string)
      free(object->datum.string);
    break;

  case SYMBOL:
    if (object->datum.symbol)
      free(object->datum.symbol);
    break;

  case CONS:
    if (object->datum.cons)
      free(object->datum.cons);
    break;

  case LAMBDA:
    if (object->datum.cons)
      free(object->datum.cons);
    break;

  default:
    break;
  }

  free(object);
}

static void unmark_all_references() {
  reference_list_t *reference = references;

  while (reference) {
    reference->node->marked = 0;
    reference = reference->next;
  }
}

static void mark(lisp_object_t *root) {
  if (root->marked)
    return;

  root->marked = 1;

  if ((root->type == CONS &&
       root != NIL) || root->type == LAMBDA) {
    mark(((cons*) root->datum.cons)->car);
    mark(((cons*) root->datum.cons)->cdr);
  } 
}

static lisp_object_t* eval_arg_list(lisp_object_t *arg_list, lisp_object_t *env) {
  lisp_object_t *arg_ptr = arg_list;

  lisp_object_t *args_cons  = make_cons(NULL, NULL);
  lisp_object_t *to_return  = args_cons;
  lisp_object_t **prev_ref  = &args_cons;

  while (arg_ptr != NIL) {
    /* eval the args in applicative order */
    lisp_object_t *arg_value = eval(CONS_VALUE(arg_ptr)->car, env);

    if (arg_value == NULL)
      return NULL;

    /* add the value to the car of the arg list*/
    CONS_VALUE(args_cons)->car = arg_value;
    prev_ref = &(CONS_VALUE(args_cons)->cdr);
    CONS_VALUE(args_cons)->cdr = make_cons(NULL, NULL);

    arg_ptr = CONS_VALUE(arg_ptr)->cdr;
    args_cons = CONS_VALUE(args_cons)->cdr;
  }

  *prev_ref = NIL;
  if (args_cons == NIL)
    return NIL;

  return to_return;
}

lisp_object_t* eval(lisp_object_t *expression, lisp_object_t *environment) {
  lisp_object_t *expr = NULL;
  lisp_object_t *super_env = NULL;
  lisp_object_t *car, *cdr;

  switch (expression->type) {

  case STRING:
    return expression;

  case SYMBOL:
    expr = get(expression, environment);

    if (expr == NULL) {
      super_env = nice_get("*lisp-super-env*", environment);

      if (super_env != NULL) expr = get(expression, super_env);
    }

    if (expr == NULL) {
      fprintf(stderr, "Error: symbol \"%s\" not bound.\n", expression->datum.symbol);
      return NULL;
    }
    return expr;

  case NUMBER:
    return expression;

  case NATIVE_FUNCTION:
    return expression;

  case CONS:
    if (expression != NIL && CONS_VALUE(expression)->car->type == SYMBOL
        && strcmp(CONS_VALUE(expression)->car->datum.symbol, "lambda") == 0) {
      expr = make_cons(expression, environment);
      expr->type = LAMBDA;

      return expr;
    } else if (expression == NIL) {
      return NIL;
    }

    car = eval(CONS_VALUE(expression)->car, environment);

    if (car == NULL)
      return NULL;

    cdr = eval_arg_list(CONS_VALUE(expression)->cdr, environment);

    if (cdr == NULL)
      return NULL;

    return apply(car, cdr);

  default:
    return NIL;
  }
}

lisp_object_t* apply(lisp_object_t *f, lisp_object_t *xargs) {
  if (f->type == NATIVE_FUNCTION) {
    return f->datum.native_func(xargs);
  } else {
    fprintf(stderr, "Error: not defined on lambda's yet.\n");
    return NIL;
  }
}

lisp_object_t* get(lisp_object_t *symbol, lisp_object_t *environment) {
  if (symbol->type != SYMBOL) {
    fprintf(stderr, "Error: get expects its first argument to be of type SYMBOL.\n");
    return NULL;
  }

  if (environment->type != CONS) {
    fprintf(stderr, "Error: get expects its second argument to be of type CONS.\n");
    return NULL;
  }

  lisp_object_t *current_node = environment;
  while (current_node != NIL) {
    lisp_object_t *symbol_pair = ((cons*) current_node->datum.cons)->car;

    if (strcmp(((cons*) symbol_pair->datum.cons)->car->datum.symbol,
               symbol->datum.symbol) == 0) {
      return ((cons*) symbol_pair->datum.cons)->cdr;
    }

    current_node = ((cons*) current_node->datum.cons)->cdr;
  }

  return NULL;
}

lisp_object_t* nice_get(char *s, lisp_object_t *environment) {
  lisp_object_t *symbol = make_lisp_object();
  symbol->type = SYMBOL;
  symbol->datum.symbol = strdup(s);

  return get(symbol, environment);
}

lisp_object_t* set(lisp_object_t *s, lisp_object_t *v, lisp_object_t *e) {
  if (s->type != SYMBOL) {
    fprintf(stderr, "Error: set expects its first argument to be of type SYMBOL.\n");
    return e;
  }

  if (e == NIL) {
    return make_cons(make_cons(s, v), NIL);
  }

  lisp_object_t *current_node = e;
  lisp_object_t **prev_ref = &current_node;
  while (current_node != NIL) {
    lisp_object_t *symbol_pair = CONS_VALUE(current_node)->car;

    if (strcmp(CONS_VALUE(symbol_pair)->car->datum.symbol, s->datum.symbol) == 0) {
      CONS_VALUE(symbol_pair)->cdr = v;
      return e;
    }

    prev_ref = &CONS_VALUE(current_node)->cdr;
    current_node = CONS_VALUE(current_node)->cdr;
  }

  if (current_node == NIL) {
    *prev_ref = make_cons(make_cons(s, v), NIL);
  }

  return e;
}

lisp_object_t* nice_set(char *symbol_name, lisp_object_t *v, lisp_object_t *e) {
  lisp_object_t *s = make_lisp_object();
  s->type = SYMBOL;
  s->datum.symbol = strdup(symbol_name);

  return set(s, v, e);
}

void register_function(char *function_name, lisp_function function,
                       lisp_object_t *environment) {
  lisp_object_t *function_o = make_lisp_object();
  function_o->datum.native_func = function;
  function_o->type = NATIVE_FUNCTION;

  nice_set(function_name, function_o, environment);
}
