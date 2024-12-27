%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "lex.yy.h"
#include "functions.h"

#define MAX_BUFFERS 16

YY_BUFFER_STATE buffers[MAX_BUFFERS];
int buffers_count = 0;
void buffersfree();
%}

%union {
    int d;
    struct ast* a;
}

%token <d> NUMBER
%token VARIABLE
%token SIN COS TG CTG ARCSIN ARCCOS ARCTG ARCCTG
%token SQRT LN E
%token EXIT CLEAR D COMMA
%token EOL

%type <a> exp

%left '+' '-'
%left '*' '/'
%left '^'
%precedence UMINUS

%destructor {
    treefree($$);
} exp

%%
calclist
    : %empty
    | calclist exp EOL {
        eval($2);
        printf("func = \n\t%s\nder  = \n\t%s\n", $2->function, $2->derivative);
        treefree($2);
    }
    | calclist D '[' exp COMMA NUMBER ']' EOL   {
        if ($6 > MAX_BUFFERS) {
            printf("Order of derivative must be less than %d; found %d\n", MAX_BUFFERS, $6);
            treefree($4);
            return 0;
        }
        if ($6 < 0) {
            printf("Order of derivative must be a nonnegative integer; found %d\n", $6);
            treefree($4);
            return 0;
        }

        eval($4);
        if ($6 == 0) {
            printf("der  = \n\t%s\n", $4->function);
            treefree($4);
            return 0;
        }
        if ($6 == 1) {
            printf("der = \n\t%s\n", $4->derivative);
            treefree($4);
            buffersfree();
            return 0;
        }

        char* input = build_string("D[%s,%d]\n", $4->derivative, $6-1);
        YY_BUFFER_STATE buffer = yy_scan_string(input);
        yy_switch_to_buffer(buffer);
        if (buffers_count <= MAX_BUFFERS) {
            buffers[++buffers_count] = buffer;
        }

        free(input);
        treefree($4);
        yyparse();
    }
    | calclist EXIT EOL                         { exit(EXIT_SUCCESS); }
    | calclist CLEAR EOL                        { system("clear"); }
    | calclist EOL                              {  }
    | error EOL                                 { yyclearin; yyerrok; }
    ;

exp
    : NUMBER                                    { $$ = newnum($1); }
    | VARIABLE                                  { $$ = newvar(); }

    | exp '+' exp                               { $$ = newast('+', $1, $3); }
    | exp '-' exp                               { $$ = newast('-', $1, $3); }
    | exp '*' exp                               { $$ = newast('*', $1, $3); }
    | exp '/' exp                               { $$ = newast('/', $1, $3); }
    | exp '^' exp                               { $$ = newast('^', $1, $3); }
    | '-' exp  %prec UMINUS                     { $$ = newast('U', $2, NULL); }

    | SIN '(' exp ')'                           { $$ = newfunc(f_sin, $3); }
    | SIN VARIABLE                              { $$ = newfunc(f_sin, newvar()); }
    | COS '(' exp ')'                           { $$ = newfunc(f_cos, $3); }
    | COS VARIABLE                              { $$ = newfunc(f_cos, newvar()); }
    | TG '(' exp ')'                            { $$ = newfunc(f_tg, $3); }
    | TG VARIABLE                               { $$ = newfunc(f_tg, newvar()); }
    | CTG '(' exp ')'                           { $$ = newfunc(f_ctg, $3); }
    | CTG VARIABLE                              { $$ = newfunc(f_ctg, newvar()); }
    | ARCSIN '(' exp ')'                        { $$ = newfunc(f_arcsin, $3); }
    | ARCSIN VARIABLE                           { $$ = newfunc(f_arcsin, newvar()); }
    | ARCCOS '(' exp ')'                        { $$ = newfunc(f_arccos, $3); }
    | ARCCOS VARIABLE                           { $$ = newfunc(f_arccos, newvar()); }
    | ARCTG '(' exp ')'                         { $$ = newfunc(f_arctg, $3); }
    | ARCTG VARIABLE                            { $$ = newfunc(f_arctg, newvar()); }
    | ARCCTG '(' exp ')'                        { $$ = newfunc(f_arcctg, $3); }
    | ARCCTG VARIABLE                           { $$ = newfunc(f_arcctg, newvar()); }

    | SQRT '(' exp ')'                          { $$ = newfunc(f_sqrt, $3); }
    | SQRT VARIABLE                             { $$ = newfunc(f_sqrt, newvar()); }
    | LN '(' exp ')'                            { $$ = newfunc(f_ln, $3); }
    | LN VARIABLE                               { $$ = newfunc(f_ln, newvar()); }
    | E '^' exp                                 { $$ = newfunc(f_e, $3); }

    | '(' exp ')'                               { $$ = $2; }
    ;
%%

int main(int argc, char* argv[]) {
    if (argc > 1) {
        FILE *file = fopen(argv[1], "r");
        if (!file) {
            perror(argv[1]);
            return 1;
        }
        yyin = file;
        
        yyparse();

        fclose(file);
        yylex_destroy();
        return 0;
    }

    char* line;
    using_history();
    read_history("history");
    while ((line = readline("> ")) != NULL) {
        if (*line) {
            add_history(line);

            char* input = build_string("%s\n", line);
            YY_BUFFER_STATE buffer = yy_scan_string(input);
            yy_switch_to_buffer(buffer);;

            yyparse();

            yy_delete_buffer(buffer);
            free(input);
        }

        printf("\n");

        free(line);
    }

    write_history("history");

    clear_history();
    rl_cleanup_after_signal();
    rl_clear_history();
    yylex_destroy();
    return 0;
}

void buffersfree() {
    for (int i = 0; i < buffers_count; i++) {
        if (buffers[i])
            yy_delete_buffer(buffers[i]);
    }
    buffers_count = 0;
}
