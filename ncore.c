#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "ncore.h"
#include "nstdlib.h"

#define LASSERT(args, cond, fmt, ...) \
    if (!(cond)) { \
        nval* err = nval_err(fmt, ##__VA_ARGS__); \
        nval_del(args); \
        return err; \
    }

#define LASSERT_TYPE(func, args, index, expect) \
    LASSERT(args, args->cell[index]->type == expect, \
        "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
        func, index, ntype_name(args->cell[index]->type), ntype_name(expect))

#define LASSERT_NUM(func, args, num) \
    LASSERT(args, args->count == num, \
        "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
        func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
    LASSERT(args, args->cell[index]->count != 0, \
        "Function '%s' passed {} for argument %i.", func, index);

/* Constuctor and destructor for environment types */
nenv* nenv_new(void) {
    nenv* e = malloc(sizeof(nenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void nenv_del(nenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        nval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

/* Environment manipulation functions */
nval* nenv_get(nenv* e, nval* k) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return nval_copy(e->vals[i]);
        }
    }
    return nval_err("Symbol '%s' not declared", k->sym);
}

void nenv_put(nenv* e, nval* k, nval* v) {
    /* Check if variable already exists */
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            nval_del(e->vals[i]);
            e->vals[i] = nval_copy(v);
            return;
        }
    }

    /* If not, create it */
    e->count++;
    e->vals = realloc(e->vals, sizeof(nval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    e->vals[e->count-1] = nval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1], k->sym);
}

/* Constructor functions for nval types */
nval* nval_num(long x) {
    nval* v = malloc(sizeof(nval));
    v->type = NVAL_NUM;
    v->num = x;
    return v;
}

nval* nval_err(char* fmt, ...) {
    nval* v = malloc(sizeof(nval));
    v->type = NVAL_ERR;

    /* Create a va list and initialize it */
    va_list va;
    va_start(va, fmt);

    /* Allocate 512 bytes of space */
    v->err = malloc(512);

    /* printf the error string with a maximum of 511 characters */
    vsnprintf(v->err, 511, fmt, va);

    /* Reallocate to number of bytes actually used */
    v->err = realloc(v->err, strlen(v->err)+1);

    /* Cleanup our va list */
    va_end(va);

    return v;
}

nval* nval_sym(char* s) {
    nval* v = malloc(sizeof(nval));
    v->type = NVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

nval* nval_sexpr(void) {
    nval* v = malloc(sizeof(nval));
    v->type = NVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

nval* nval_qexpr(void) {
    nval* v = malloc(sizeof(nval));
    v->type = NVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

nval* nval_fun(nbuiltin func) {
    nval* v = malloc(sizeof(nval));
    v->type = NVAL_FUN;
    v->fun = func;
    return v;
}

/* nval manipulation functions */
void nval_del(nval* v) {
    switch (v->type) {
        /* Number and function, nothing special */
        case NVAL_NUM: break;
        case NVAL_FUN: break;
        /* err and sym are strings with malloc */
        case NVAL_ERR: free(v->err); break;
        case NVAL_SYM: free(v->sym); break;

        /* S/Q-expression, delete all elements inside */
        case NVAL_QEXPR:
        case NVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                nval_del(v->cell[i]);
            }
            /* Free mem to contain the pointers */
            free(v->cell);
        break;
    }
    /* Free mem allocated for v itself */
    free(v);
}

nval* nval_add(nval* v, nval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(nval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

nval* nval_pop(nval* v, int i) {
    nval* x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i+1], sizeof(nval*) * (v->count-i-1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(nval*) * v->count);
    return x;
}

nval* nval_take(nval* v, int i) {
    nval* x = nval_pop(v, i);
    nval_del(v);
    return x;
}

nval* nval_join(nval* x, nval* y) {
    while (y->count) {
        x = nval_add(x, nval_pop(y, 0));
    }
    nval_del(y);
    return x;
}

nval* nval_copy(nval* v) {
    nval* x = malloc(sizeof(nval));
    x->type = v->type;

    switch (v->type) {
        case NVAL_FUN: x->fun = v->fun; break;
        case NVAL_NUM: x->num = v->num; break;

        case NVAL_ERR:
            x->err = malloc(strlen(v->err)+1);
            strcpy(x->err, v->err); break;
        case NVAL_SYM:
            x->sym = malloc(strlen(v->sym)+1);
            strcpy(x->sym, v->sym); break;

        case NVAL_SEXPR:
        case NVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(nval*) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = nval_copy(v->cell[i]);
            }
        break;
    }
    return x;
}

char* ntype_name(int t) {
    switch(t) {
        case NVAL_FUN: return "Function";
        case NVAL_NUM: return "Number";
        case NVAL_ERR: return "Error";
        case NVAL_SYM: return "Symbol";
        case NVAL_SEXPR: return "S-Expression";
        case NVAL_QEXPR: return "Q-Expression";
        default: return "Unknown";
    }
}

void nenv_add_builtin(nenv* e, char* name, nbuiltin func) {
    nval* k = nval_sym(name);
    nval* v = nval_fun(func);
    nenv_put(e, k, v);
    nval_del(k);
    nval_del(v);
}

void nenv_add_builtins(nenv* e) {
    /* Variable Functions */
    nenv_add_builtin(e, "def", builtin_def);

    /* List Functions */
    nenv_add_builtin(e, "list", builtin_list);
    nenv_add_builtin(e, "head", builtin_head);
    nenv_add_builtin(e, "tail", builtin_tail);
    nenv_add_builtin(e, "eval", builtin_eval);
    nenv_add_builtin(e, "join", builtin_join);

    /* Mathematical Functions */
    nenv_add_builtin(e, "+", builtin_add);
    nenv_add_builtin(e, "-", builtin_sub);
    nenv_add_builtin(e, "*", builtin_mul);
    nenv_add_builtin(e, "/", builtin_div);
}

/* Builtin arithmatic operations */
nval* builtin_op(nenv* e, nval* a, char* op) {
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE(op, a, i, NVAL_NUM);
    }

    nval* x = nval_pop(a, 0);

    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }

    while (a->count > 0) {
        nval* y = nval_pop(a, 0);

        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                nval_del(x);
                nval_del(y);
                x = nval_err("Division By Zero!"); break;
            }
            x->num /= y->num;
        }
        if (strcmp(op, "%") == 0) {
            if (y->num == 0) {
                nval_del(x);
                nval_del(y);
                x = nval_err("Division By Zero!"); break;
            }
            x->num %= y->num;
        }
        if (strcmp(op, "min") == 0) { x->num = min(x->num, y->num); }
        if (strcmp(op, "max") == 0) { x->num = max(x->num, y->num); }

        nval_del(y);
    }

    nval_del(a);
    return x;
}

nval* builtin_add(nenv* e, nval* a) {
    return builtin_op(e, a, "+");
}

nval* builtin_sub(nenv* e, nval* a) {
    return builtin_op(e, a, "-");
}

nval* builtin_mul(nenv* e, nval* a) {
    return builtin_op(e, a, "*");
}

nval* builtin_div(nenv* e, nval* a) {
    return builtin_op(e, a, "/");
}

/* Take a Q-Expression and return a Q-Expression with only the first element */
nval* builtin_head(nenv* e, nval* a) {
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, NVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);

    nval* v = nval_take(a, 0);
    while (v->count > 1) {
        nval_del(nval_pop(v, 1));
    }
    return v;
}

/* Take a Q-Expression and return a Q-Expression with the first element removed */
nval* builtin_tail(nenv* e, nval* a) {
    LASSERT_NUM("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, NVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", a, 0);

    nval* v = nval_take(a, 0);
    nval_del(nval_pop(v, 0));
    return v;
}

/* Convert S-expression to Q-expression */
nval* builtin_list(nenv* e, nval* a) {
    a->type = NVAL_QEXPR;
    return a;
}

/* Evaluate Q-expression like S-express */
nval* builtin_eval(nenv* e, nval* a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, NVAL_QEXPR);

    nval* x = nval_take(a, 0);
    x->type = NVAL_SEXPR;
    return nval_eval(e, x);
}

/* Join multiple Q-expressions into one */
nval* builtin_join(nenv* e, nval* a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT(a, a->cell[i]->type == NVAL_QEXPR,
            "Function 'join' was passed incorrect type");
    }

    nval* x = nval_pop(a, 0);
    while (a->count) {
        x = nval_join(x, nval_pop(a, 0));
    }
    nval_del(a);
    return x;
}

nval* builtin_def(nenv* e, nval* a) {
    LASSERT(a, a->cell[0]->type == NVAL_QEXPR,
        "Function 'def' passed incorrect type");

    nval* syms = a->cell[0];
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, syms->cell[i]->type == NVAL_SYM,
            "Function 'def' cannot define non-symbol");
    }

    LASSERT(a, syms->count == a->count -1,
        "Function 'def' cannot define incorrect number of values to symbols");

    for (int i = 0; i < syms->count; i++) {
        nenv_put(e, syms->cell[i], a->cell[i+1]);
    }
    nval_del(a);
    return nval_sexpr();
}

/* Core print statements */
void nval_print(nval* v) {
    switch (v->type) {
        case NVAL_NUM:   printf("%li", v->num); break;
        case NVAL_ERR:   printf("Error: %s", v->err); break;
        case NVAL_SYM:   printf("%s", v->sym); break;
        case NVAL_SEXPR: nval_expr_print(v, '(', ')'); break;
        case NVAL_QEXPR: nval_expr_print(v, '{', '}'); break;
        case NVAL_FUN:   printf("<function>"); break;
    }
}

void nval_println(nval* v) {
    nval_print(v);
    putchar('\n');
}

void nval_expr_print(nval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        nval_print(v->cell[i]);

        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

/* Code evaluation functions */
nval* nval_eval(nenv* e, nval* v) {
    if (v->type == NVAL_SYM) {
        nval* x = nenv_get(e, v);
        nval_del(v);
        return x;
    }
    if (v->type == NVAL_SEXPR) {
        return nval_eval_sexpr(e, v);
    }
    return v;
}

nval* nval_eval_sexpr(nenv* e, nval* v) {
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = nval_eval(e, v->cell[i]);
    }

    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == NVAL_ERR) {
            return nval_take(v, i);
        }
    }

    if (v->count == 0) { return v; }

    if (v->count == 1) { return nval_take(v, 0); }

    nval* f = nval_pop(v, 0);
    if (f->type != NVAL_FUN) {
        nval_del(f); nval_del(v);
        return nval_err("First element is not a function");
    }

    nval* result = f->fun(e, v);
    nval_del(f);
    return result;
}
