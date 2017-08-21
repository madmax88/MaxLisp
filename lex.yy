%{
#include "reader.h"
%}

%option noyywrap

%x string

NUMBER     [0-9]+\.?[0-9]*

QUOTE       \"

LEFT_PAREN  \(

RIGHT_PAREN \)

SYMBOL     [^\t\n\r \"\'\`\;\(\)]+

%%

{NUMBER} {
  return TOKEN_NUMBER;
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
