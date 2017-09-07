#ifndef READER_H
#define READER_H

#include <stdio.h>
#include "lisp.h"

enum token {TOKEN_NUMBER, TOKEN_SYMBOL, TOKEN_QUOTE,
            TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_EOF,
            TOKEN_SINGLE_QUOTE, TOKEN_DOT, TOKEN_COMMA,
            TOKEN_COMMA_AT, TOKEN_BACKQUOTE};

enum read_condition {READ_EOF = 1, READ_UNBALANCED_PAREN, READ_DOT};

lisp_object_t* read_object(FILE *input, char *prompt);

#endif
