#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "mpc.h"
#include "builtins.h"
#include "ncore.h"
#include "mempool.h"

void nenv_add_builtin(nenv* e, char* name, nbuiltin func) {
    nval* k = nval_sym(name);
    nval* v = nval_fun(func);
    nenv_put_protected(e, k, v);
    nval_del(k);
    nval_del(v);
}

void nenv_add_builtin_macro(nenv* e, char* name, nbuiltin func) {
    nval* k = nval_sym(name);
    nval* v = nval_macro(func);
    nenv_put_protected(e, k, v);
    nval_del(k);
    nval_del(v);
}

void nenv_add_builtins(nenv* e) {
    nenv_add_builtin(e, "load", builtin_load);
    nenv_add_builtin(e, "print", builtin_print);
    nenv_add_builtin(e, "error", builtin_error);
    nenv_add_builtin(e, "exit", builtin_exit);

    /* Variables and functions */
    nenv_add_builtin_macro(e, "def", builtin_def);
    nenv_add_builtin_macro(e, "pdef", builtin_pdef);
    nenv_add_builtin_macro(e, "=", builtin_put);
    nenv_add_builtin(e, "undef", builtin_undef);
    nenv_add_builtin(e, "\\", builtin_lambda);

    /* List Functions */
    nenv_add_builtin(e, "list", builtin_list);
    nenv_add_builtin(e, "head", builtin_head);
    nenv_add_builtin(e, "tail", builtin_tail);
    nenv_add_builtin(e, "eval", builtin_eval);
    nenv_add_builtin(e, "join", builtin_join);

    nenv_add_builtin(e, "strcat", builtin_strconcat);
    nenv_add_builtin(e, "mem-pool-stats", builtin_pool_stats);

    /* Mathematical Functions */
    nenv_add_builtin(e, "+", builtin_add);
    nenv_add_builtin(e, "-", builtin_sub);
    nenv_add_builtin(e, "*", builtin_mul);
    nenv_add_builtin(e, "/", builtin_div);
    nenv_add_builtin(e, "%", builtin_modulus);

    /* Logical operators */
    nenv_add_builtin(e, "if", builtin_if);
    nenv_add_builtin(e, "==", builtin_eq);
    nenv_add_builtin(e, "!=", builtin_ne);
    nenv_add_builtin(e, ">",  builtin_gt);
    nenv_add_builtin(e, "<",  builtin_lt);
    nenv_add_builtin(e, ">=", builtin_ge);
    nenv_add_builtin(e, "<=", builtin_le);
}

nval* builtin_pool_stats(nenv* e, nval* a) {
    pool_stats();
    nval_del(a);
    return nval_empty();
}

nval* builtin_load(nenv* e, nval* a) {
  LASSERT_NUM("load", a, 1);
  LASSERT_TYPE("load", a, 0, NVAL_STR);
  
  /* Parse File given by string name */
  mpc_result_t r;
  if (mpc_parse_contents(a->cell[0]->str, Nitrogen, &r)) {
    
    /* Read contents */
    nval* expr = nval_read(r.output);
    mpc_ast_delete(r.output);

    /* Evaluate each Expression */
    while (expr->count) {
        nval* x = nval_eval(e, nval_pop(expr, 0));
        /* If Evaluation leads to error print it */
        if (x->type == NVAL_ERR) { nval_println(x); }
        /* Special case for NVAL_QUIT type */
        if (x->type == NVAL_QUIT) { nval_del(x); break; }
        nval_del(x);
    }
    
    /* Delete expressions and arguments */
    nval_del(expr);    
    nval_del(a);
    
    /* Return empty list */
    return nval_ok();
    
  } else {
    /* Get Parse Error as String */
    char* err_msg = mpc_err_string(r.error);
    mpc_err_delete(r.error);
    
    /* Create new error message using it */
    nval* err = nval_err("Could not load Library %s", err_msg);
    free(err_msg);
    nval_del(a);
    
    /* Cleanup and return error */
    return err;
  }
}

/* Builtin arithmatic operations */
nval* builtin_op(nenv* e, nval* a, char* op) {
    LASSERT_MIN_ARGS(op, a, 2);
    bool is_double = false;

    for (int i = 0; i < a->count; i++) {
        LASSERT(a, a->cell[i]->type == NVAL_NUM || a->cell[i]->type == NVAL_DOUBLE,
            "Function '%s' was passed incorrect type", op);

        if (a->cell[i]->type == NVAL_DOUBLE) {
            is_double = true;
            break;
        }
    }

    /* Convert numbers to double for operation */
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type == NVAL_NUM) {
            a->cell[i]->doub = a->cell[i]->num;
            a->cell[i]->type = NVAL_DOUBLE;
        }
    }
    nval* x = nval_pop(a, 0);

    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->doub = -x->doub;
    }

    while (a->count > 0) {
        nval* y = nval_pop(a, 0);

        if (strcmp(op, "+") == 0) { x->doub += y->doub; }
        if (strcmp(op, "-") == 0) { x->doub -= y->doub; }
        if (strcmp(op, "*") == 0) { x->doub *= y->doub; }
        if (strcmp(op, "/") == 0) {
            if (y->doub == 0) {
                nval_del(x);
                nval_del(y);
                x = nval_err("Division By Zero!"); break;
            }
            x->doub /= y->doub;
        }
        if (strcmp(op, "%") == 0) {
            if (y->doub == 0) {
                nval_del(x);
                nval_del(y);
                x = nval_err("Division By Zero!"); break;
            }
            x->doub = fmod(x->doub, y->doub);
        }

        nval_del(y);
    }

    /* If none of the inputs were double, reconvert to NVAL_NUM */
    if (!is_double) {
        x->num = x->doub;
        x->type = NVAL_NUM;
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

nval* builtin_modulus(nenv* e, nval* a) {
    return builtin_op(e, a, "%");
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

/* String concatenation */
nval* builtin_strconcat(nenv* e, nval* a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT(a, a->cell[i]->type == NVAL_STR,
            "Function 'strcon' was passed incorrect type");
    }

    int full_length = 0;
    for (int i = 0; i < a->count; i++) {
        full_length += strlen(a->cell[i]->str);
    }

    if (full_length > 0) {
        char full_string[full_length];
        strcpy(full_string, a->cell[0]->str);
        for (int i = 1; i < a->count; i++) {
            strcat(full_string, a->cell[i]->str);
        }
        nval_del(a);
        return nval_str(full_string);
    } else {
        return nval_err("String length is 0");
    }
}

nval* builtin_def(nenv* e, nval* a) {
    return builtin_var(e, a, "def");
}

nval* builtin_pdef(nenv* e, nval* a) {
    return builtin_var(e, a, "pdef");
}

nval* builtin_put(nenv* e, nval* a) {
    return builtin_var(e, a, "=");
}

nval* builtin_var(nenv* e, nval* a, char* func) {
    LASSERT_NUM(func, a, 2);
    if (a->cell[0]->type != NVAL_SYM && a->cell[0]->type != NVAL_SEXPR) {
        LASSERT_TYPE(func, a, 0, NVAL_QEXPR);

        nval* syms = a->cell[0];
        for (int i = 0; i < syms->count; i++) {
            LASSERT(a, (syms->cell[i]->type == NVAL_SYM),
              "Function '%s' cannot define non-symbol. "
              "Got %s, Expected %s.", func, 
              ntype_name(syms->cell[i]->type),
              ntype_name(NVAL_SYM));
        }

        LASSERT(a, (syms->count == a->count-1),
            "Function '%s' passed too many arguments for symbols. "
            "Got %i, Expected %i.", func, syms->count, a->count-1);

        for (int i = 0; i < syms->count; i++) {
            a->cell[i+1] = nval_eval(e, a->cell[i+1]);
            /* If 'def' define in globally. If 'put' define in locally */
            if (strcmp(func, "def") == 0) {
                if (!nenv_def(e, syms->cell[i], a->cell[i+1])) {
                    nval_del(a);
                    return nval_err("Cannot redefine protected functions");
                }
            }

            if (strcmp(func, "pdef") == 0) {
                if (!nenv_def_protected(e, syms->cell[i], a->cell[i+1])) {
                    nval_del(a);
                    return nval_err("Cannot redefine protected functions");
                }
            }

            if (strcmp(func, "=")   == 0) {
                nenv_put(e, syms->cell[i], a->cell[i+1]);
            } 
        }
    } else {
        LASSERT_NUM(func, a, 2);
        if (a->cell[0]->type == NVAL_SEXPR) {
            nval* pre_result = nval_eval(e, a->cell[0]);
            a->cell[0] = nval_pop(pre_result, 0);
            nval_del(pre_result);
        }

        if (a->cell[1]->type == NVAL_SEXPR) {
            a->cell[1] = nval_eval(e, a->cell[1]);
        }

        if (strcmp(func, "def") == 0) {
            if (!nenv_def(e, a->cell[0], a->cell[1])) {
                nval_del(a);
                return nval_err("Cannot redefine protected functions");
            }
        }

        if (strcmp(func, "pdef") == 0) {
            if (!nenv_def_protected(e, a->cell[0], a->cell[1])) {
                nval_del(a);
                return nval_err("Cannot redefine protected functions");
            }
        }

        if (strcmp(func, "=") == 0) {
            nenv_put(e, a->cell[0], a->cell[1]);
        }
    }

    nval_del(a);
    return nval_empty();
}

nval* builtin_undef(nenv* e, nval* a) {
    LASSERT_NUM("undef", a, 1);
    LASSERT(a, a->cell[0]->type == NVAL_QEXPR,
        "Function 'undef' passed incorrect type");

    nval* syms = a->cell[0];
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, syms->cell[i]->type == NVAL_SYM,
            "Function 'undef' cannot define non-symbol");
    }

    LASSERT_NUM("undef", a, 1);

    for (int i = 0; i < syms->count; i++) {
        nenv_rem(e, syms->cell[i]);
    }
    nval_del(a);
    return nval_empty();
}

nval* builtin_lambda(nenv* e, nval* a) {
    LASSERT_NUM("\\", a, 2);
    LASSERT_TYPE("\\", a, 0, NVAL_QEXPR);
    LASSERT_TYPE("\\", a, 1, NVAL_QEXPR);

    for (int i = 0; i < a->cell[0]->count; i++) {
        LASSERT(a, (a->cell[0]->cell[i]->type == NVAL_SYM),
            ntype_name(a->cell[0]->cell[i]->type), ntype_name(NVAL_SYM));
    }

    nval* formals = nval_pop(a, 0);
    nval* body = nval_pop(a, 0);
    nval_del(a);
    return nval_lambda(formals, body);
}

nval* builtin_gt(nenv* e, nval* a) {
    return builtin_ord(e, a, ">");
}

nval* builtin_lt(nenv* e, nval* a) {
    return builtin_ord(e, a, "<");
}

nval* builtin_ge(nenv* e, nval* a) {
    return builtin_ord(e, a, ">=");
}

nval* builtin_le(nenv* e, nval* a) {
    return builtin_ord(e, a, "<=");
}

nval* builtin_ord(nenv* e, nval* a, char* op) {
    LASSERT_NUM(op, a, 2);
    LASSERT(a, a->cell[0]->type == NVAL_NUM || a->cell[0]->type == NVAL_DOUBLE,
        "Function '%s' cannot work on non-numbers");
    LASSERT(a, a->cell[1]->type == NVAL_NUM || a->cell[1]->type == NVAL_DOUBLE,
        "Function '%s' cannot work on non-numbers");

    /* Convert to double for comparison */
    if (a->cell[0]->type == NVAL_NUM) {
        a->cell[0]->doub = a->cell[0]->num;
    }
    if (a->cell[1]->type == NVAL_NUM) {
        a->cell[1]->doub = a->cell[1]->num;
    }

    int r;
    if (strcmp(op, ">") == 0) {
        r = (a->cell[0]->doub > a->cell[1]->doub);
    }
    if (strcmp(op, "<") == 0) {
        r = (a->cell[0]->doub < a->cell[1]->doub);
    }
    if (strcmp(op, ">=") == 0) {
        r = (a->cell[0]->doub >= a->cell[1]->doub);
    }
    if (strcmp(op, "<=") == 0) {
        r = (a->cell[0]->doub <= a->cell[1]->doub);
    }
    nval_del(a);
    return nval_num(r);
}

int nval_eq(nval* x, nval* y) {

  if (x->type != y->type) { return 0; }

  switch (x->type) {
    case NVAL_QUIT:
    case NVAL_NUM: return (x->num == y->num);
    case NVAL_DOUBLE: return (x->doub == y->doub);

    case NVAL_ERR: return (strcmp(x->err, y->err) == 0);
    case NVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
    case NVAL_STR: return (strcmp(x->str, y->str) == 0);

    case NVAL_FUN:
      if (x->builtin || y->builtin) {
        return x->builtin == y->builtin;
      } else {
        return nval_eq(x->formals, y->formals) 
          && nval_eq(x->body, y->body);
      }

    case NVAL_QEXPR:
    case NVAL_SEXPR:
      if (x->count != y->count) { return 0; }
      for (int i = 0; i < x->count; i++) {
        if (!nval_eq(x->cell[i], y->cell[i])) { return 0; }
      }
      return 1;
    break;
  }
  return 0;
}

nval* builtin_cmp(nenv* e, nval* a, char* op) {
    LASSERT_NUM(op, a, 2);
    int r;
    if (strcmp(op, "==") == 0) {
        r = nval_eq(a->cell[0], a->cell[1]);
    }
    if (strcmp(op, "!=") == 0) {
        r = !nval_eq(a->cell[0], a->cell[1]);
    }
    nval_del(a);
    return nval_num(r);
}

nval* builtin_eq(nenv* e, nval* a) {
    return builtin_cmp(e, a, "==");
}

nval* builtin_ne(nenv* e, nval* a) {
    return builtin_cmp(e, a, "!=");
}

nval* builtin_if(nenv* e, nval* a) {
    LASSERT_MIN_ARGS("if", a, 2);
    LASSERT_TYPE("if", a, 0, NVAL_NUM);
    LASSERT_TYPE("if", a, 1, NVAL_QEXPR);
    if (a->count > 2) {
        LASSERT_TYPE("if", a, 2, NVAL_QEXPR);
    }

    nval* x;
    a->cell[1]->type = NVAL_SEXPR;
    if (a->count > 2) {
        a->cell[2]->type = NVAL_SEXPR;
    }

    /* Only truth evaluation is given */
    if (a->cell[0]->num) {
        x = nval_eval(e, nval_pop(a, 1));
    } 

    if (!a->cell[0]->num && a->count > 2) {
        /* Both evaluations are given and is false */
        x = nval_eval(e, nval_pop(a, 2));
    } 
    if (!a->cell[0]->num && a->count < 2) {
        /* False evaluation was NOT given */
        x = nval_qexpr();
    }

    nval_del(a);
    return x;
}

nval* builtin_print(nenv* e, nval* a) {
    for (int i = 0; i < a->count; i++) {
        nval_print(a->cell[i]); putchar(' ');
    }
    putchar('\n');
    nval_del(a);
    return nval_ok();
}

nval* builtin_error(nenv* e, nval* a) {
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, NVAL_STR);
    nval* err = nval_err(a->cell[0]->str);
    nval_del(a);
    return err;
}

nval* builtin_exit(nenv* e, nval* a) {
    long errnum = 0;
    if (a->count > 0) {
        nval_println(a);
        errnum = a->cell[0]->num;
    }
    nval_del(a);
    return nval_quit(errnum);
}
