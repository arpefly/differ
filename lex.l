%{
#include "yacc.tab.h"
#include <stdio.h>
%}

%option noyywrap

%%
[0-9]+      { yylval.d = atoi(yytext); return NUMBER; }

[-+*/\^\(\)\[\]]    { return yytext[0]; }

"x"                 { return VARIABLE; }

"sin"               { return SIN; }
"cos"               { return COS; }
"tg"                { return TG; }
"ctg"               { return CTG; }
"arcsin"            { return ARCSIN; }
"arccos"            { return ARCCOS; }
"arctg"             { return ARCTG; }
"arcctg"            { return ARCCTG; }
"sqrt"              { return SQRT; }
"ln"                { return LN; }
"e"                 { return E; }

"exit"              { return EXIT; }
"clear"             { return CLEAR; }
[Dd]                { return D; }
","                 { return COMMA; }

\n                  { return EOL; }
[ \t]               {  }
.                   {  }
%%
