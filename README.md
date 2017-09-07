# MaxLisp

MaxLisp is a small Lisp implementation.

It is:
* a Lisp-1
* lexically scoped
* extremely simple

## Examples

See `src/core.lisp` to see how the core language is defined using a few based primitives. 

## Known issues

* macro expansion is terribly slow
* garbage collection is correct, but better rules should be defined rather than collecting naively

## More Goals

I'm busy, so this all may or may not happen:
* multithreaded GC
* profile the macro-expansion process to determine bottlenecks
* first-class continutations
