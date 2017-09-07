%{
#include "reader.h"
%}

%option noyywrap

%x string

NUMBER       [0-9]+\.?[0-9]*

DOT          \.

QUOTE        \"

LEFT_PAREN   \(

RIGHT_PAREN  \)

SYMBOL       [^\t\n\r \"\'\`\;\(\)\,\@]+

SINGLE_QUOTE \'

COMMENT      \;

COMMA        \,

COMMA_AT     \,\@

BACKQUOTE    \`

%%

{COMMENT}.*\n {

}

{COMMA} {
  return TOKEN_COMMA;
}

{COMMA_AT} {
  return TOKEN_COMMA_AT;
}

{BACKQUOTE} {
  return TOKEN_BACKQUOTE;
}

{SINGLE_QUOTE} {
  return TOKEN_SINGLE_QUOTE;
}

{NUMBER} {
  return TOKEN_NUMBER;
}

{DOT} {
  return TOKEN_DOT;
}

{SYMBOL} {
  return TOKEN_SYMBOL;
}

{QUOTE}([^\"]|(\\\")?)*{QUOTE} {
  return TOKEN_QUOTE;
}

{LEFT_PAREN} {
  return TOKEN_LEFT_PAREN;
}

{RIGHT_PAREN} {
  return TOKEN_RIGHT_PAREN;
}

[ \t\n]* {

}

<<EOF>> { return TOKEN_EOF; }

%%

void my_savior(int c, char *text) {
     yyunput(c, text);
}

void switch_buffer(FILE *file) {
     YY_BUFFER_STATE buffer_state = yy_create_buffer(file, YY_BUF_SIZE);
     yy_switch_to_buffer(buffer_state);
}