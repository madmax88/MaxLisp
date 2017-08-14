#include <stdio.h>
#include <stdlib.h>

#include "lisp.h"
#include "reader.h"

extern lisp_object_t *NIL;

int main() {
  init_lisp_module();

  while (1) {
    lisp_object_t *object = read_object(stdin);
    lisp_object_t *pro = print_object(object);

    printf("%s\n", pro->datum.string);
  }
}
