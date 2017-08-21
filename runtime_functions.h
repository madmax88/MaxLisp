#ifndef RUNTIME_FUNC_H
#define RUNTIME_FUNC_H

lisp_object_t* quote_func(lisp_object_t *args);

lisp_object_t* if_func(lisp_object_t *args, lisp_object_t *environment);

lisp_object_t* cons_func(lisp_object_t *args);

lisp_object_t* car_func(lisp_object_t *args);

lisp_object_t* cdr_func(lisp_object_t *args);

lisp_object_t* set_func(lisp_object_t *expression, lisp_object_t *env);

#endif
