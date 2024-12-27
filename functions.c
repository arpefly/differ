#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "functions.h"

ast* newast(int nodetype, ast* l, ast* r) 
{
    ast* a = malloc(sizeof(ast));

    if (!a) {
        printf("out of memory. malloc ast");
        exit(1);
    }

    a->nodetype = nodetype;
    a->function = NULL;
    a->derivative = NULL;
    a->l = l;
    a->r = r;

    return a;
}

ast* newnum(int value)
{
    number* num = malloc(sizeof(number));

    if (!num) {
        printf("out of memory. malloc num");
        exit(1);
    }

    num->nodetype = 'N';
    num->function = build_string("%d", value);    
    num->derivative = strdup("0");
    num->value = value;

    return (ast*)num;
}

ast* newvar()
{
    variable* var = malloc(sizeof(variable));

    if (!var) {
        printf("out of memory. malloc var");
        exit(1);
    }

    var->nodetype = 'X';
    var->function = strdup("x");
    var->derivative = strdup("1");

    return (ast*)var;
}

ast* newfunc(int functype, ast* arg) 
{
    function* func = malloc(sizeof(function));

    if (!func) {
        printf("out of memory. malloc func");
        exit(1);
    }

    func->nodetype = 'F';
    func->function = NULL;
    func->derivative = NULL;
    func->functype = functype;
    func->arg = arg;

    return (ast*)func;
}


char* build_string(char* format, ...)
{
    va_list args;

    va_start(args, format);
    size_t size = vsnprintf(NULL, 0, format, args) + 1; // + 1 байт для EOL (\0)
    va_end(args);

    char* result = (char*)malloc(size);
    if (!result) {
        fprintf(stderr, "out of memory when allocating in build_string method.\n");
        exit(EXIT_FAILURE);
    }

    va_start(args, format);
    vsnprintf(result, size, format, args);
    va_end(args);

    return result;
}


void unary_minus(ast* a)
{
    eval(a->l);
    
    a->function = build_string("-(%s)", a->l->function); // Строка самой функции: -(u)
    
    a->derivative = build_string("-(%s)", a->l->derivative); // Строка производной: -(u')
}

void sum_rule(ast* a)
{
    // Производная суммы: d(u + v) = du + dv
    eval(a->l);
    eval(a->r);

    // Оба операнда числа. Свёртывание текущего узла до числа (nodetype = 'N'). value = ((number*)a->l)->value + ((number*)a->r)->value
    if (a->l->nodetype == 'N' && a->r->nodetype == 'N') {
        int sum = ((struct number*)a->l)->value + ((struct number*)a->r)->value;

        treefree(a->l);
        treefree(a->r);
        
        a->nodetype = 'N';
        a->function = build_string("%d", sum);
        a->derivative = strdup("0");
        ((struct number*)a)->value = sum;
        return;
    }

    // Упрощение, если один из операндов равен 0
    if (a->l->nodetype == 'N' && ((struct number*)a->l)->value == 0) {
        a->function = build_string("%s", a->r->function);
        a->derivative =  build_string("%s", a->r->derivative);
        return;
    }
    if (a->r->nodetype == 'N' && ((struct number*)a->r)->value == 0) {
        a->function = build_string("%s", a->l->function);
        a->derivative =  build_string("%s", a->l->derivative);
        return;
    }

    // Строка функции
    a->function = build_string("(%s %c %s)", a->l->function, a->nodetype, a->r->function);

    // Строка производной
    a->derivative = build_string("(%s %c %s)", a->l->derivative, a->nodetype, a->r->derivative);
}

void product_rule(ast* a)
{
    // Производная произведения: (u*v)' = u'*v + u*v'
    eval(a->l);
    eval(a->r);

    // Строка функции
    // Оба операнда числа. Свёртывание текущего узла до числа (nodetype = 'N'). value = ((number*)a->l)->value * ((number*)a->r)->value
    if (a->l->nodetype == 'N' && a->r->nodetype == 'N') {
        int product = ((number*)a->l)->value * ((number*)a->r)->value;

        treefree(a->l);
        treefree(a->r);

        a->nodetype = 'N';
        a->function = build_string("%d", product);
        a->derivative = strdup("0");
        ((number*)a)->value = product;
        return;
    }

    // 0*x = x*0 = 0. Свёртывание текущего узла до числа (nodetype = 'N'). value = ((number*)a->l)->value * ((number*)a->r)->value
    if ((a->l->nodetype == 'N' && ((number*)a->l)->value == 0) || (a->r->nodetype == 'N' && ((number*)a->r)->value == 0)) {
        treefree(a->l);
        treefree(a->r);

        a->nodetype = 'N';
        a->function = strdup("0");
        a->derivative = strdup("0");
        ((number*)a)->value = 0;
        return;
    }
        
    if (a->l->nodetype == 'N' && ((number*)a->l)->value == 1) // 1*x = x
        a->function = build_string("%s", a->r->function);
    else if (a->r->nodetype == 'N' && ((number*)a->r)->value == 1) // x*1 = x
        a->function = build_string("%s", a->l->function);
    else // Общий случай
        a->function = build_string("%s*%s", a->l->function, a->r->function);

    // Строка производной
    if ((a->l->nodetype == 'N' && ((number*)a->l)->value == 0) || (a->r->nodetype == 'N' && ((number*)a->r)->value == 0)) // (0*x)' = (x*0)' = 0
        a->derivative = strdup("0");
    else if (a->l->nodetype == 'N' && ((number*)a->l)->value == 1) // (1*x)' = 0*x + 1*x' = x'
        a->derivative = build_string("%s", a->r->derivative);
    else if (a->r->nodetype == 'N' && ((number*)a->r)->value == 1) // (x*1)' = x'*1 + x*0 = x'
        a->derivative = build_string("%s", a->l->derivative);
    else if (strcmp(a->l->derivative, "0") == 0 && strcmp(a->r->derivative, "0") == 0) // u' = 0 and v' = 0 => (u*v)' = 0
        a->derivative = strdup("0");
    else if (strcmp(a->l->derivative, "0") == 0) // u' = 0 => (u*v)' = u*v'
        a->derivative = build_string("%s*%s", a->l->function, a->r->derivative);
    else if (strcmp(a->r->derivative, "0") == 0) // v' = 0 => (u*v)' = u'*v 
        a->derivative = build_string("%s*%s", a->l->derivative, a->r->function);
    else if (a->l->nodetype == 'N') // (num*x)' = 0*x + num*x' = num*x'
        a->derivative = build_string("%d*%s", ((number*)a->l)->value, a->r->derivative);
    else if (a->r->nodetype == 'N') // (x*num)' = x'*num + x*0 = x'*num
        a->derivative = build_string("%s*%d", a->l->derivative, ((number*)a->r)->value);
    else {
        // Общий случай: u'*v + u*v'
        char* lhs = build_string("(%s*%s)", a->l->derivative, a->r->function);
        char* rhs = build_string("(%s*%s)", a->l->function, a->r->derivative);
        a->derivative = build_string("(%s + %s)", lhs, rhs);

        free(lhs);
        free(rhs);
    }
}

void quotient_rule(ast* a)
{
    // (u/v)э = (u'*v - u*v') / (u^2)
    eval(a->l);
    eval(a->r);

    // Строка самой функции
    a->function = build_string("(%s/%s)", a->l->function, a->r->function);

    // Строка числителя производной: u'*v - u*v'
    char* numerator_lhs = build_string("%s*%s", a->l->derivative, a->r->function);
    char* numerator_rhs = build_string("%s*%s", a->l->function, a->r->derivative);
    char* numerator = build_string("(%s - %s)", numerator_lhs, numerator_rhs);

    // Строка знаменателя производной: v^2
    char* denominator = build_string("(%s)^2", a->r->function);

    // Строка полной производной: (числитель / знаменатель)
    a->derivative = build_string("%s/(%s)", numerator, denominator);

    free(numerator_lhs);
    free(numerator_rhs);    
    free(numerator);
    free(denominator);
}

void power_rule(ast* a)
{
    // (u^v)' = u^v * [v'*ln(u) + v*u'/u]
    ast* base = a->l;
    ast* exponent = a->r;

    eval(base);
    eval(exponent);

    // Частный случай если степень - это число
    if (exponent->nodetype == 'N') {
        int power = ((number*)exponent)->value;

        if (power == 0) { // x^0 = 1; (x^0)' = (1)' = 0
            treefree(base);
            treefree(exponent);

            a->nodetype = 'N';
            a->function = strdup("1");
            a->derivative = strdup("0");
            ((number*)a)->value = 1;
            return;
        }

        if (power == 1) { // x^1 = x; (x^1)' = x'
            a->function = build_string("(%s)", base->function);
            a->derivative = build_string("(%s)", base->derivative);
            return;
        }
        // Строка функции u^v
        a->function = build_string("(%s^%s)", base->function, exponent->function);
        if (power == 2) // (x^2)' = 2*x'
            a->derivative = build_string("(%d*(%s)*%s)", power, base->function, base->derivative);
        else // Общий случай
            a->derivative = build_string("(%d*(%s^%d)*%s)", ((number*)exponent)->value, base->function, ((number*)exponent)->value-1, base->derivative);
    }
    // Общий случай
    else {
        // Строка функции u^v
        a->function = build_string("(%s)^(%s)", base->function, exponent->function);

        // Строка производной: v'*ln(u) + v*u'/u
        char* left_term = build_string("(%s*ln(%s))", exponent->derivative, base->function);
        char* right_term = build_string("(%s*%s/%s)", exponent->function, base->derivative, base->function);

        // Полная строка производной: u^v * (left_term + right_term)
        a->derivative = build_string("(%s*(%s + %s))", a->function, left_term, right_term);

        free(left_term);
        free(right_term);
    }
}

void chain_rule(ast* outer, ast* inner, char* outer_func, char* outer_der)
{
    // (f(g(x)))' = f'(g(x))*g'(x)
    eval(inner);

    // Строка функции внешнего узла: f(g(x))
    outer->function = build_string("%s(%s)", outer_func, inner->function);

    // Строка производной внешнего узла: f'(g(x))*g'(x)
    char* format = '\0';
    if (strcmp(inner->derivative, "0") == 0)
        format = "0";
    else if (strcmp(inner->derivative, "1") == 0)
        format = "%s";
    else
        format = "%s*%s";

    char* outer_term = build_string(format, outer_der, inner->derivative);
    outer->derivative = build_string(outer_term, inner->function, inner->derivative);

    free(outer_term);
}


void eval(ast* a)
{
    switch (a->nodetype) {
        case 'U': // Унарный минус
            unary_minus(a);            
            break;
        case 'N': // Лист дерева
        case 'X': // Лист дерева
            break;
        case '+':
        case '-': // Производная суммы: (u + v)' = u' + v'
            sum_rule(a);
            break;
        case '*': // Производная произведения: (u*v)' = u'*v + u*v'
            product_rule(a);
            break;
        case '/': // (u/v)' = (u'*v - u*v') / (u^2)
            quotient_rule(a);
            break;
        case '^': // (u^v)' = u^v * [v'*ln(u) + v*u'/u]
            power_rule(a); 
            break;
        case 'F': // (f(g(x)))' = f'(g(x))*g'(x)
            function* func = ((function*)a);
            if (func->functype == f_sin) chain_rule(a, func->arg, "sin", "cos(%s)");
            else if (func->functype == f_cos) chain_rule(a, func->arg, "cos", "-sin(%s)");
            else if (func->functype == f_tg) chain_rule(a, func->arg, "tg", "1/cos(%s)^2");
            else if (func->functype == f_ctg) chain_rule(a, func->arg, "ctg", "(-1/sin(%s)^2)");
            else if (func->functype == f_arcsin) chain_rule(a, func->arg, "arcsin", "1/(sqrt(1-(%s)^2))");
            else if (func->functype == f_arccos) chain_rule(a, func->arg, "arccos", "(-1/(sqrt(1-(%s)^2)))");
            else if (func->functype == f_arctg) chain_rule(a, func->arg, "arctg", "1/(1+(%s)^2)");
            else if (func->functype == f_arcctg) chain_rule(a, func->arg, "arcctg", "(-1/(1+(%s)^2))");
            else if (func->functype == f_sqrt) chain_rule(a, func->arg, "sqrt", "1/(2*sqrt(%s))");
            else if (func->functype == f_ln) chain_rule(a, func->arg, "ln", "1/(%s)");
            else if (func->functype == f_e) chain_rule(a, func->arg, "e^", "e^(%s)");
            break;
        default:
            printf("bad node at eval. nodetype: %c\n", a->nodetype);
            break;
    }
}

void treefree(ast *a)
{
    switch(a->nodetype) {
        case 'U':
            free(a->function);
            free(a->derivative);
            treefree(a->l);
            free(a);
            break;
        case 'F':
            free(a->function);
            free(a->derivative);
            treefree(((function*)a)->arg);
            free(a);
            break;
        case 'N':
        case 'X':
            free(a->function);
            free(a->derivative);
            free(a);
            break;
        case '+':
        case '-':
        case '*':
        case '/':
        case '^':
            free(a->function);
            free(a->derivative);
            treefree(a->l);
            treefree(a->r);
            free(a);
            break;
        default:
            printf("[ERROR]: free bad node %c\n", a->nodetype);
            break;
    }
}


void yyerror(char *s, ...) {
    va_list ap;
    va_start(ap, s);
    
    fprintf(stderr, "[ERROR] :: ");
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
}
