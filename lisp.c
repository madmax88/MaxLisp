#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lisp.h"

static reference_list_t *references = NULL;
lisp_object_t* NIL = NULL;

static void unmark_all_references();
static void mark(lisp_object_t *object);

void init_lisp_module() {
  NIL = make_cons(NULL, NULL);
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
    src->marked = 1;
    dest->datum.cons = make_cons(deep_copy(src_cons->car), deep_copy(src_cons->cdr));
    break;

  case LAMBDA:
    src_cons = (cons*) src->datum.cons;
    dest->datum.cons = make_cons(deep_copy(src_cons->car), deep_copy(src_cons->cdr));
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
  lisp_object_t* lisp_string = make_lisp_object();
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
    temp = snprintf(dest, string_size, "%s", object->datum.string);

    if (temp >= string_size) {
      string_size = temp + 1;
      dest = realloc(dest, string_size);
      memset(dest, 0, string_size);
      strlen = snprintf(dest, string_size, "%s", object->datum.string);
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
      strcpy(dest, "NIL");
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
  while (!references->node->marked) {
    reference_list_t *next_ptr = references->next;
    delete_object(references->node);
    free(references);
    references = next_ptr;
  }

  reference_list_t *previous_reference = references;
  reference_list_t *current_reference = references->next;

  /* Finally, we can iterate through the rest of the list */
  while (current_reference) {
    reference_list_t *next_reference = current_reference->next;

    if (!current_reference->node->marked) {
      delete_object(current_reference->node);
      free(current_reference);
      previous_reference->next = next_reference;
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
