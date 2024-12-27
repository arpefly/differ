typedef struct ast {
    int nodetype;
    char* function;
    char* derivative;
    struct ast* l;
    struct ast* r;
} ast;

typedef struct number {
    int nodetype;
    char* function;
    char* derivative;
    int value;
} number;

typedef struct variable {
    int nodetype;
    char* function;
    char* derivative;
} variable;

typedef struct function {
    int nodetype;
    char* function;
    char* derivative;
    int functype;
    ast* arg;
} function;

ast* newast(int nodetype, ast* l, ast* r);
ast* newnum(int num);
ast* newvar();
ast* newfunc(int functype, ast* arg);
char* build_string(char* format_string, ...);
void eval(ast* a);
void treefree(ast *a);

enum FunctionTypes {
    f_sin,
    f_cos,
    f_tg,
    f_ctg,
    f_arcsin,
    f_arccos,
    f_arctg,
    f_arcctg,
    f_ln,
    f_sqrt,
    f_e
};

void yyerror(char* s, ...);
