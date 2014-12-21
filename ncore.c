#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include "ncore.h"
#include "builtins.h"
#include "mpc.h"
#include "mempool.h"

/* Constuctor and destructor for environment types */
nenv* nenv_new(void) {
    nenv* e = malloc(sizeof(nenv));
    e->par = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    e->protected = NULL;
    return e;
}

void nenv_del(nenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        nval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e->protected);
    free(e);
}

/* Environment manipulation functions */
nval* nenv_get(nenv* e, nval* k) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return nval_copy(e->vals[i]);
        }
    }

    /* Check all parent environments if symbol not found */
    if (e->par) {
        return nenv_get(e->par, k);
    }
    return nval_err("Symbol '%s' not declared", k->sym);
}

bool nenv_put(nenv* e, nval* k, nval* v) {
    /* Check if variable already exists */
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            if (!e->protected[i]) {
                nval_del(e->vals[i]);
                e->vals[i] = nval_copy(v);
                return true;
            }
            return false;
        }
    }

    /* If not, create it */
    e->count++;
    e->vals = realloc(e->vals, sizeof(nval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    e->vals[e->count-1] = nval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1], k->sym);
    return true;
}

bool nenv_put_protected(nenv* e, nval* k, nval* v) {
    /* Check if variable already exists */
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return false;
        }
    }

    /* If not, create it */
    e->count++;
    e->vals = realloc(e->vals, sizeof(nval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);
    e->protected = realloc(e->protected, sizeof(bool) * e->count);

    e->vals[e->count-1] = nval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    e->protected[e->count-1] = true;
    strcpy(e->syms[e->count-1], k->sym);
    return true;
}

void nenv_rem(nenv* e, nval* k) {
    do {
        for (int i = 0; i < e->count; i++) {
            if (strcmp(e->syms[i], k->sym) == 0) {
                free(e->syms[i]);
                nval_del(e->vals[i]);
                return;
            }
        }
        e = e->par;
    } while (e->par);
    return;
}

nenv* nenv_copy(nenv* e) {
    nenv* n = malloc(sizeof(nenv));
    n->par = e->par;
    n->count = e->count;
    if (e->count > 0) {
        n->syms = malloc(sizeof(char*) * n->count);
        n->vals = malloc(sizeof(nval*) * n->count);
        for (int i = 0; i < e->count; i++) {
            n->syms[i] = malloc(strlen(e->syms[i]) + 1);
            strcpy(n->syms[i], e->syms[i]);
            n->vals[i] = nval_copy(e->vals[i]);
            n->protected[i] = e->protected[i];
        }
    } else {
        n->syms = NULL;
        n->vals = NULL;
        n->protected = NULL;
    }
    return n;
}

/* Define variable in global scope */
bool nenv_def(nenv* e, nval* k, nval* v) {
    while (e->par) { e = e->par; }
    return nenv_put(e, k, v);
}

bool nenv_def_protected(nenv* e, nval* k, nval* v) {
    while (e->par) { e = e->par; }
    return nenv_put_protected(e, k, v);
}

/* Constructor functions for nval types */
nval* nval_num(long x) {
    nval* v = nmalloc();
    v->type = NVAL_NUM;
    v->num = x;
    return v;
}

nval* nval_double(double x) {
    nval* v = nmalloc();
    v->type = NVAL_DOUBLE;
    v->doub = x;
    return v;
}

nval* nval_err(char* fmt, ...) {
    nval* v = nmalloc();
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
    nval* v = nmalloc();
    v->type = NVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

nval* nval_sexpr(void) {
    nval* v = nmalloc();
    v->type = NVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

nval* nval_qexpr(void) {
    nval* v = nmalloc();
    v->type = NVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

nval* nval_fun(nbuiltin func) {
    nval* v = nmalloc();
    v->type = NVAL_FUN;
    v->builtin = func;
    return v;
}

nval* nval_macro(nbuiltin func) {
    nval* v = nval_fun(func);
    v->type = NVAL_FUN_MACRO;
    return v;
}

nval* nval_lambda(nval* formals, nval* body) {
    nval* v = nmalloc();
    v->type = NVAL_FUN;
    v->builtin = NULL;
    v->env = nenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}

nval* nval_str(char* s) {
    nval* v = nmalloc();
    v->type = NVAL_STR;
    v->str = malloc(strlen(s)+1);
    strcpy(v->str, s);
    return v;
}

nval* nval_ok(void) {
    nval* v = nmalloc();
    v->type = NVAL_OK;
    v->ok = true;
    return v;
}

nval* nval_empty(void) {
    nval* v = nmalloc();
    v->type = NVAL_EMPTY;
    return v;
}

nval* nval_quit(long x) {
    nval* v = nmalloc();
    v->type = NVAL_QUIT;
    v->num = x;
    return v;
}

/* nval manipulation functions */
void nval_del(nval* v) {
    switch (v->type) {
        /* Number and function, nothing special */
        case NVAL_NUM: break;
        case NVAL_DOUBLE: break;
        case NVAL_OK:  break;
        case NVAL_EMPTY: break;
        case NVAL_FUN_MACRO: break; /* Macros are only builtin systems */
        /* err and sym are strings with malloc */
        case NVAL_ERR: free(v->err); break;
        case NVAL_SYM: free(v->sym); break;
        case NVAL_STR: free(v->str); break;

        /* S/Q-expression, delete all elements inside */
        case NVAL_QEXPR:
        case NVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                nval_del(v->cell[i]);
            }
            /* Free mem to contain the pointers */
            free(v->cell);
        break;

        /* User defined functions */
        case NVAL_FUN:
            if (!v->builtin) {
                nenv_del(v->env);
                nval_del(v->formals);
                nval_del(v->body);
            }
        break;
    }
    /* Free mem allocated for v itself */
    nfree(v);
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
    nval* x = nmalloc();
    x->type = v->type;

    switch (v->type) {
        case NVAL_EMPTY: break;
        case NVAL_NUM: x->num = v->num; break;
        case NVAL_DOUBLE: x->doub = v->doub; break;
        case NVAL_OK:  x->ok = v->ok; break;
        case NVAL_FUN_MACRO: x->builtin = v->builtin; break;

        case NVAL_ERR:
            x->err = malloc(strlen(v->err)+1);
            strcpy(x->err, v->err); break;
        case NVAL_SYM:
            x->sym = malloc(strlen(v->sym)+1);
            strcpy(x->sym, v->sym); break;
        case NVAL_STR:
            x->str = malloc(strlen(v->str)+1);
            strcpy(x->str, v->str); break;

        case NVAL_SEXPR:
        case NVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(nval*) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = nval_copy(v->cell[i]);
            }
        break;

        case NVAL_FUN:
            if (v->builtin) {
                x->builtin = v->builtin;
            } else {
                x->builtin = NULL;
                x->env = nenv_copy(v->env);
                x->formals = nval_copy(v->formals);
                x->body = nval_copy(v->body);
            }
        break;
    }
    return x;
}

char* ntype_name(int t) {
    switch(t) {
        case NVAL_FUN: return "Function";
        case NVAL_FUN_MACRO: return "Language Macro";
        case NVAL_NUM: return "Number";
        case NVAL_DOUBLE: return "Double";
        case NVAL_ERR: return "Error";
        case NVAL_SYM: return "Symbol";
        case NVAL_SEXPR: return "S-Expression";
        case NVAL_QEXPR: return "Q-Expression";
        case NVAL_STR: return "String";
        case NVAL_OK: return "Ok";
        case NVAL_EMPTY: return "Empty";
        case NVAL_QUIT: return "Exit command";
        default: return "Unknown";
    }
}

/* Core print statements */
void nval_print(nval* v) {
    switch (v->type) {
        case NVAL_EMPTY: break;
        case NVAL_NUM:   printf("%li", v->num); break;
        case NVAL_DOUBLE:   printf("%f", v->doub); break;
        case NVAL_ERR:   printf("Error: %s", v->err); break;
        case NVAL_SYM:   printf("%s", v->sym); break;
        case NVAL_OK:
            if (v->ok) {
                printf("Ok");
            } else {
                printf("Not ok");
            }
            break;
        case NVAL_SEXPR: nval_expr_print(v, '(', ')'); break;
        case NVAL_QEXPR: nval_expr_print(v, '{', '}'); break;
        case NVAL_STR: nval_print_str(v); break;
        case NVAL_FUN_MACRO:
            if (v->builtin) {
                printf("<macro>");
            }
            break;
        case NVAL_FUN:
            if (v->builtin) {
                printf("<builtin>");
            } else {
                printf("(\\ "); nval_print(v->formals);
                putchar(' '); nval_print(v->body); putchar(')');
            }
            break;
    }
}

void nval_println(nval* v) {
    nval_print(v);
    if (v->type != NVAL_EMPTY) {
        putchar('\n');
    }
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

void nval_print_str(nval* v) {
    char* escaped = malloc(strlen(v->str)+1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);
    free(escaped);
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
    if (v->count == 0) { return v; }

    v->cell[0] = nval_eval(e, v->cell[0]);
    if (v->cell[0]->type != NVAL_FUN_MACRO) {
        for (int i = 1; i < v->count; i++) {
            v->cell[i] = nval_eval(e, v->cell[i]);
        }

        for (int i = 0; i < v->count; i++) {
            if (v->cell[i]->type == NVAL_ERR) {
                return nval_take(v, i);
            }
        }

        if (v->count == 1) {
            if (v->cell[0]->type != NVAL_FUN) {
                return nval_take(v, 0);
            }
        }
    }

    nval* f = nval_pop(v, 0);
    if (f->type != NVAL_FUN && f->type != NVAL_FUN_MACRO) {
        nval* err = nval_err(
            "S-Expression starts with incorrect type. "
            "Got %s, Expected %s.",
            ntype_name(f->type), ntype_name(NVAL_FUN));
        nval_del(f); nval_del(v);
        return err;
    }

    nval* result = nval_call(e, f, v);
    nval_del(f);
    return result;
}

nval* nval_call(nenv* e, nval* f, nval* a) {
    if (f->builtin) { return f->builtin(e, a); }

    int given = a->count;
    int total = f->formals->count;

    while (a->count) {
        if (f->formals->count == 0) {
            nval_del(a);
            return nval_err(
                "Function passed too many arguments. "
                "Got %i, Expected %i.", given, total); 
        }

        nval* sym = nval_pop(f->formals, 0);
        /*Special case to deal with & */
        if (strcmp(sym->sym, "&") == 0) {
            if (f->formals->count != 1) {
                nval_del(a);
                return nval_err("Function format invalid. Symbol '&' not followed by single symbol.");
            }

            nval* nsym = nval_pop(f->formals, 0);
            nenv_put(f->env, nsym, builtin_list(e, a));
            nval_del(sym); nval_del(nsym);
            break;
        }
        nval* val = nval_pop(a, 0);
        nenv_put(f->env, sym, val);
        nval_del(sym); nval_del(val);
    }

    nval_del(a);

    if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {
        if (f->formals->count != 2) {
            return nval_err("Function format invalid. "
                "Symbol '&' not followed by single symbol.");
        }

        nval_del(nval_pop(f->formals, 0));
        nval* sym = nval_pop(f->formals, 0);
        nval* val = nval_qexpr();
        nenv_put(f->env, sym, val);
        nval_del(sym); nval_del(val);
    }

    if (f->formals->count == 0) {
        f->env->par = e;
        return builtin_eval(f->env, nval_add(nval_sexpr(), nval_copy(f->body)));
    } else {
        return nval_copy(f);
    }
}
