#include <stdio.h>
#include <stdlib.h>

#include "lisp.h"
#include "reader.h"

extern lisp_object_t *NIL;
extern lisp_object_t *T;

int main() {
  lisp_object_t *global_environment = init_lisp_module();

  while (1) {
    do_gc(global_environment);

    lisp_object_t *object = read_object(stdin);

    if (object == NULL)
      break;

    lisp_object_t *new_object = eval(object, global_environment);

    if (new_object == NULL)
      continue;

    lisp_object_t *pro = print_object(new_object);

    printf("%s\n", pro->datum.string);
  }
}
