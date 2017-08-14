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

{NUMBER} { printf("I see a number\n"); return TOKEN_NUMBER;}

{SYMBOL} { printf("I see a symbol\n"); return TOKEN_SYMBOL;}

{QUOTE}  { // printf("I see a start quote\n"); 
           BEGIN(string);}

<string>\\{QUOTE} {
    // printf("I see a quoted-quote!");
    yymore();
}

<string>{QUOTE} {
  //printf("I see an end-quote!\n");
  BEGIN(0);
  return TOKEN_QUOTE;
};

<string>.* {
  yymore();
}

{QUOTE}([^\"]|(\\\")?)*{QUOTE} {
    printf("I'm a string");
}

{LEFT_PAREN} { printf("I see a left parenthesis"); return TOKEN_LEFT_PAREN;}

{RIGHT_PAREN} { printf("I see a right parenthesis"); return TOKEN_RIGHT_PAREN;}

<<EOF>> { return TOKEN_EOF; }

%%

int main() {
    yylex();    
}
