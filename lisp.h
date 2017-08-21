#ifndef LISP_H
#define LISP_H

#define CONS_VALUE(x) (((cons*) x->datum.cons))

typedef enum {
  SYMBOL,
  NUMBER,
  STRING,
  CONS,
  LAMBDA,
  MACRO,
  NATIVE_FUNCTION
} lisp_type;

struct lisp_object;
struct cons_struct;

typedef struct lisp_object* (*lisp_function) (struct lisp_object *param_list);

struct lisp_object {
  lisp_type type;
  unsigned char marked;

  union {
    double number;
    char *string;
    char *symbol;
    struct cons_struct *cons;  /* so we can forward reference cons */
    lisp_function native_func;
  } datum;
};

typedef struct lisp_object lisp_object_t;

typedef struct cons_struct {
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
lisp_object_t* init_lisp_module();

/* Returns a deep copy of a lisp_object */
lisp_object_t* deep_copy(lisp_object_t *src);

/* Runs garbage collection. */
void do_gc(lisp_object_t *environment);

/* Returns a string representing the given object */
lisp_object_t* print_object(lisp_object_t *object);

/* Core function: eval 
 *
 * An environment is a linked list of pairs relating symbols to values,
 * i.e. ((foo . 10) (bar . buzz) ...)
 */
lisp_object_t* eval(lisp_object_t *expression, lisp_object_t *environment);

/* Core function: apply
 *
 * Apply's a function (even native) to its arguments
 */
lisp_object_t* apply(lisp_object_t *f, lisp_object_t *xargs, lisp_object_t *env);

/* Returns the value that symbol is bound to in environment
 * 
 * If no value is found, NULL (not NIL) is returned. 
 */
lisp_object_t* get(lisp_object_t *symbol, lisp_object_t *environment);

lisp_object_t* nice_get(char *s, lisp_object_t *e);

/* Sets a symbol to a value in the current environment
 */
lisp_object_t* set(lisp_object_t *s, lisp_object_t *v, lisp_object_t *e);

lisp_object_t* nice_set(char *symbol_name, lisp_object_t *v, lisp_object_t *e);

/* registers a function with the runtime
 */
void register_function(char *function_name, lisp_function function,
                       lisp_object_t *environment);

/* returns the # of allocated objects */
size_t allocated_objects();

#endif
