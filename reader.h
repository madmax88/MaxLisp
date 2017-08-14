#ifndef READER_H
#define READER_H

#include <stdio.h>
#include "lisp.h"

enum token {TOKEN_NUMBER, TOKEN_SYMBOL, TOKEN_QUOTE,
            TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_EOF};

lisp_object_t* read_object(FILE *input);

#endif
