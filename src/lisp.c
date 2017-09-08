#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lisp.h"
#include "runtime_functions.h"

static reference_list_t *references = NULL;

lisp_object_t* NIL                = NULL;
lisp_object_t* T                  = NULL;

static void unmark_all_references();
static void mark(lisp_object_t *object);

static lisp_object_t* eval_arg_list(lisp_object_t *arg_list, lisp_object_t *env);

static lisp_object_t* apply_lambda(lisp_object_t *lambda_expr, lisp_object_t *xargs);

void* xmalloc(size_t bytes) {
  char *object = malloc(bytes);

  if (!object) {
    fprintf(stderr, "Error: out of memory.\n");
    exit(1);
  }
  
  return object;
}

lisp_object_t* init_lisp_module() {
  NIL = make_cons(NULL, NULL);

  lisp_object_t *global_environment = NIL;
  lisp_object_t *core_path = make_lisp_object();
  core_path->type = STRING;
  core_path->datum.string = strdup("core.lisp");

  T = make_lisp_object();
  T->type = SYMBOL;
  T->datum.symbol = strdup("t");
  
  global_environment = nice_set("nil", NIL, global_environment);
  nice_set("t", T, global_environment);

  /* these functions are defined in runtime_functions.h */
  register_function("cons", cons_func, global_environment);
  register_function("car",  car_func,  global_environment);
  register_function("cdr",  cdr_func,  global_environment);
  register_function("list", list, global_environment);
  register_function("length", length, global_environment);
  register_function("eq", eq, global_environment);
  register_function("atom?", atomp, global_environment);
  register_function("primitive-print", primitive_print, global_environment);

  /* we need to load "core.lisp" as part of the bootstrap process */
  load(make_cons(core_path, NIL), global_environment);

  return global_environment;
}

static reference_list_t* make_reference_list(lisp_object_t *object) {
  reference_list_t* ref = xmalloc(sizeof(reference_list_t));
  ref->next = NULL;
  ref->node = object;
  return ref;
}

static void create_reference(lisp_object_t *object) {
  reference_list_t *new_reference = make_reference_list(object);
  new_reference->next = references;

  references = new_reference;
}

lisp_object_t* make_lisp_object() {
  lisp_object_t *object = xmalloc(sizeof(lisp_object_t));
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
  object->datum.cons = xmalloc(sizeof(cons));
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
  char *dest = xmalloc(string_size);
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

  case LAMBDA:
    dest = strdup("LAMBDA_CLOSURE");
    break;

  case MACRO:
    dest = strdup("META_LAMBDA_CLOSURE");
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

  case NATIVE_FUNCTION:
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
  if (!root)
    return;

  if (root->marked)
    return;

  root->marked = 1;

  if ((root->type == CONS &&
       root != NIL) || root->type == LAMBDA || root->type == MACRO) {
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
  lisp_object_t *car;

  switch (expression->type) {

  case STRING:
    return expression;

  case SYMBOL:
    expr = get(expression, environment);

    if (expr == NULL) {
      super_env = nice_get("*lisp-super-env*", environment);

      if (super_env != NULL) {
        expr = eval(expression, super_env);
      }
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

  case LAMBDA:
    return expression;

  case CONS:
    /* handles our special cases */
    if (expression == NIL) {
      return NIL;
    } else if (CONS_VALUE(expression)->car->type == SYMBOL
               && strcmp(CONS_VALUE(expression)->car->datum.symbol, "lambda") == 0) {
      expr = make_cons(expression, environment);
      expr->type = LAMBDA;
      return expr;
    } else if (CONS_VALUE(expression)->car->type == SYMBOL
               && strcmp(CONS_VALUE(expression)->car->datum.symbol, "meta-lambda") == 0) {
      expr = make_cons(expression, environment);
      expr->type = MACRO;
      return expr;
    } else if (CONS_VALUE(expression)->car->type == SYMBOL
               && strcmp(CONS_VALUE(expression)->car->datum.symbol, "quote") == 0) {
      return quote_func(CONS_VALUE(expression)->cdr);
    } else if (CONS_VALUE(expression)->car->type == SYMBOL
               && strcmp(CONS_VALUE(expression)->car->datum.symbol, "set") == 0) {
      return set_func(expression, environment);
    } else if (CONS_VALUE(expression)->car->type == SYMBOL
               && strcmp(CONS_VALUE(expression)->car->datum.symbol, "if") == 0) {
      return if_func(CONS_VALUE(expression)->cdr, environment);
    } else if (CONS_VALUE(expression)->car->type == SYMBOL
               && strcmp(CONS_VALUE(expression)->car->datum.symbol, "eval") == 0) {
      if (CONS_VALUE(expression)->cdr == NIL) {
        fprintf(stderr, "Error: eval requires 1 argument, but received 0.\n");
        return NULL;
      }

      return eval(eval(CONS_VALUE(CONS_VALUE(expression)->cdr)->car, environment),
                  environment);
    } else if (CONS_VALUE(expression)->car->type == SYMBOL
               && strcmp(CONS_VALUE(expression)->car->datum.symbol, "load") == 0) {
      lisp_object_t *xargs = eval_arg_list(CONS_VALUE(expression)->cdr, environment);
      return load(xargs, environment);
    } else if (CONS_VALUE(expression)->car->type == SYMBOL
               && strcmp(CONS_VALUE(expression)->car->datum.symbol, "apply") == 0) {
      if (CONS_VALUE(expression)->cdr == NIL) {
        fprintf(stderr, "Error: apply requires 2 arguments, but received 0.\n");
        return NULL;
      } else if (CONS_VALUE(CONS_VALUE(expression)->cdr)->cdr == NIL) {
        fprintf(stderr, "Error: apply requires 2 arguments, but received 1.\n");
        return NULL;
      }

      lisp_object_t *quote_obj = make_lisp_object();
      quote_obj->type = SYMBOL;
      quote_obj->datum.symbol = strdup("quote");
      
      lisp_object_t *args = eval(CONS_VALUE(CONS_VALUE(CONS_VALUE(expression)->cdr)->cdr)->car, environment);

      if (!args)
        return NULL;
      
      lisp_object_t *real_args = make_cons(NULL, NULL);
      lisp_object_t **last_ref = &real_args;
      lisp_object_t *arg_it = args;
      lisp_object_t *real_args_it = real_args;

      while (arg_it != NIL) {
        lisp_object_t *quoted_arg = make_cons(quote_obj,
                                              make_cons(CONS_VALUE(arg_it)->car,
                                                        NIL));
        CONS_VALUE(real_args_it)->car = quoted_arg;
        arg_it = CONS_VALUE(arg_it)->cdr;
        last_ref = &CONS_VALUE(real_args_it)->cdr;
        CONS_VALUE(real_args_it)->cdr = make_cons(NULL, NULL);
        real_args_it = CONS_VALUE(real_args_it)->cdr;
      }
      *last_ref = NIL;
      
      lisp_object_t *f = eval(CONS_VALUE(CONS_VALUE(expression)->cdr)->car, environment);

      if (!f)
        return NULL;
      
      return apply(f, real_args, environment);
    }

    car = eval(CONS_VALUE(expression)->car, environment);

    if (car == NULL)
      return NULL;

    return apply(car, CONS_VALUE(expression)->cdr, environment);

  default:
    return NIL;
  }
}

lisp_object_t* apply(lisp_object_t *f, lisp_object_t *xargs, lisp_object_t *env) {
  if (f->type == NATIVE_FUNCTION) {
    lisp_object_t *cdr = eval_arg_list(xargs, env);

    if (!cdr)
      return NULL;

    return f->datum.native_func(cdr);
  } else if (f->type == LAMBDA) {
    lisp_object_t *cdr = eval_arg_list(xargs, env);

    if (!cdr)
      return NULL;

    return apply_lambda(f, cdr);
  } else if (f->type == MACRO) {
    lisp_object_t *expansion = apply_lambda(f, xargs);

    if (!expansion)
      return NULL;

    return eval(expansion, env);
  } else {
    fprintf(stderr, "Error: unknown type to apply.\n");
    return NIL;
  }
}

static lisp_object_t* apply_lambda(lisp_object_t *lambda_expr,
                                   lisp_object_t *xargs) {
  lisp_object_t *lambda_object = CONS_VALUE(lambda_expr)->car;
  lisp_object_t *lexical_env = CONS_VALUE(lambda_expr)->cdr;
  lisp_object_t *lambda_list = NULL;
  lisp_object_t *lambda_body = NULL;
  lisp_object_t *lambda_env = NIL;

  if (CONS_VALUE(CONS_VALUE(lambda_object)->cdr)->car->type != CONS) {
    fprintf(stderr, "Error: lambda is missing a lambda list.\n");
    return NULL;
  }

  lambda_list = CONS_VALUE(CONS_VALUE(lambda_object)->cdr)->car;
  lambda_body = CONS_VALUE(CONS_VALUE(lambda_object)->cdr)->cdr;
  lambda_env  = nice_set("*lisp-super-env*", lexical_env, lambda_env);

  /* first, we create a new environment w/ variables bound properly */
  lisp_object_t *param_nav = lambda_list;
  lisp_object_t *arg_nav = xargs;
  while (param_nav != NIL && arg_nav != NIL &&
         param_nav->type == CONS && arg_nav->type == CONS) {
    lambda_env = set(CONS_VALUE(param_nav)->car, CONS_VALUE(arg_nav)->car, lambda_env);

    param_nav = CONS_VALUE(param_nav)->cdr;
    arg_nav = CONS_VALUE(arg_nav)->cdr;
  }

  /* then we have a dotted list */
  if (param_nav != NIL && param_nav->type == SYMBOL) {
    lambda_env = set(param_nav, arg_nav, lambda_env);
    param_nav = NIL;
    arg_nav = NIL;
  } else if (param_nav != NIL) {
    fprintf(stderr, "Error: badly named function parameter.\n");
    return NULL;
  }
  
  if (param_nav != NIL) {
    fprintf(stderr, "Error: too few parameters supplied to function.\n");
    return NULL;
  } else if (arg_nav != NIL) {
    fprintf(stderr, "Error: too many parameters supplied to function.\n");
    return NULL;
  }

  lisp_object_t *lambda_it = lambda_body;
  lisp_object_t *result = NIL;

  /* iterate over the lambda body, evaluating it in-order */
  while (lambda_it != NIL && lambda_it->type == CONS) {
    result = eval(CONS_VALUE(lambda_it)->car, lambda_env);
    lambda_it = CONS_VALUE(lambda_it)->cdr;
  }

  return result;
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
