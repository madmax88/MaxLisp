#ifndef LISP_H
#define LISP_H

typedef enum {
  SYMBOL,
  NUMBER,
  STRING,
  CONS,
  LAMBDA
} lisp_type;


typedef struct {
  lisp_type type;
  unsigned char marked;

  union {
    double number;
    char *string;
    char *symbol;
    void *cons;                 /* so we can forward reference cons */
  } datum;
} lisp_object_t;

typedef struct {
  lisp_object_t *car;
  lisp_object_t *cdr;
} cons;

struct reference_list_t {
  lisp_object_t *node;
  struct reference_list_t *next;
};
typedef struct reference_list_t reference_list_t;

/* Returns a pointer to a lisp_object_t that has been
   properly registered with the runtime. 
*/
lisp_object_t* make_lisp_object();

/* Frees the memory from the object (does not remove from
   reference list, though!) */
void delete_object(lisp_object_t *object);

/* Returns a cons of the two objects */
lisp_object_t* make_cons(lisp_object_t *car, lisp_object_t *cdr);

/* Must be called before using the lisp module */
void init_lisp_module();

/* Returns a deep copy of a lisp_object */
lisp_object_t* deep_copy(lisp_object_t *src);

/* Runs garbage collection. */
void do_gc(lisp_object_t *environment);

/* Returns a string representing the given object */
lisp_object_t* print_object(lisp_object_t *object);

#endif
