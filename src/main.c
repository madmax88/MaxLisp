#include <stdio.h>
#include <stdlib.h>

#include "lisp.h"
#include "reader.h"

extern lisp_object_t *NIL;
extern lisp_object_t *T;

extern enum read_condition read_flag;

int main() {
  lisp_object_t *global_environment = init_lisp_module();

  while (1) {
    do_gc(global_environment);
    printf("Total number of objects: %ld\n", allocated_objects());

    lisp_object_t *object = read_object(stdin, "> ");

    if (read_flag == READ_UNBALANCED_PAREN) {
      fprintf(stderr, "Error: unbalanced parenthesis.\n");
      continue;
    } else if (read_flag == READ_DOT) {
      fprintf(stderr, "Error: unexpected '.'.\n");
      continue;
    }
    
    if (object == NULL)
      break;

    lisp_object_t *new_object = eval(object, global_environment);

    if (new_object == NULL)
      continue;

    lisp_object_t *pro = print_object(new_object);

    printf("%s\n", pro->datum.string);
    printf("Total number of objects: %ld\n\n", allocated_objects());
    fflush(stdout);
  }
}
