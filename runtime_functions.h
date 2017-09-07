#ifndef RUNTIME_FUNC_H
#define RUNTIME_FUNC_H

lisp_object_t* quote_func(lisp_object_t *args);

lisp_object_t* if_func(lisp_object_t *args, lisp_object_t *environment);

lisp_object_t* cons_func(lisp_object_t *args);

lisp_object_t* car_func(lisp_object_t *args);

lisp_object_t* cdr_func(lisp_object_t *args);

lisp_object_t* set_func(lisp_object_t *expression, lisp_object_t *env);

lisp_object_t* list(lisp_object_t *args);

lisp_object_t* length(lisp_object_t *args);

lisp_object_t* eq(lisp_object_t *args);

lisp_object_t* load(lisp_object_t *path, lisp_object_t *env);

lisp_object_t* atomp(lisp_object_t *args);

lisp_object_t* primitive_print(lisp_object_t *args);

#endif
