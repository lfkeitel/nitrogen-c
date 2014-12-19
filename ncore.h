#ifndef ncore
#define ncore
#include <stdbool.h>

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

#define LASSERT_MIN_ARGS(func, args, num) \
    LASSERT(args, args->count >= num, \
        "Function '%s' passed incorrect number of arguments. Got %i, Needs at least %i.", \
        func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
    LASSERT(args, args->cell[index]->count != 0, \
        "Function '%s' passed {} for argument %i.", func, index);

struct nval;
struct nenv;
typedef struct nval nval;
typedef struct nenv nenv;

enum { NVAL_NUM, NVAL_ERR, NVAL_SYM, NVAL_STR, NVAL_SEXPR, NVAL_QEXPR, NVAL_FUN, NVAL_FUN_MACRO, NVAL_OK, NVAL_EMPTY, NVAL_QUIT };
/* Notes: NVAL_QUIT is a special type that when encountered will stop execution and close the interpreter. It holds no value. */

typedef nval*(*nbuiltin)(nenv*, nval*);

struct nval {
    int type;

    long num;
    char* err;
    char* sym;
    char* str;

    nbuiltin builtin;
    nenv* env;
    nval* formals;
    nval* body;
    bool ok;

    int count;
    nval** cell;
};

struct nenv {
    nenv* par;
    int count;
    char** syms;
    nval** vals;
    bool* protected;
};
/* Constuctor and destructor for environment types */
nenv* nenv_new(void);
void nenv_del(nenv* e);

/* environment manipulation functions */
nval* nenv_get(nenv* e, nval* k);
bool nenv_put(nenv* e, nval* k, nval* v);
bool nenv_put_protected(nenv* e, nval* k, nval* v);
void nenv_rem(nenv* e, nval* k);
nenv* nenv_copy(nenv* e);
bool nenv_def(nenv* e, nval* k, nval* v);
bool nenv_def_protected(nenv* e, nval* k, nval* v);

/* Constructor functions for nval types */
nval* nval_num(long x);
nval* nval_err(char* fmt, ...);
nval* nval_sym(char* s);
nval* nval_sexpr(void);
nval* nval_qexpr(void);
nval* nval_fun(nbuiltin func);
nval* nval_macro(nbuiltin func);
nval* nval_lambda(nval* formals, nval* body);
nval* nval_str(char* s);
nval* nval_ok(void);
nval* nval_empty(void);
nval* nval_quit(long x);

/* nval manipulation functions */
void nval_del(nval* v);
nval* nval_add(nval* v, nval* x);
nval* nval_pop(nval* v, int i);
nval* nval_take(nval* v, int i);
nval* nval_join(nval* x, nval* y);
nval* nval_copy(nval* v);
char* ntype_name(int t);

/* Core print statements */
void nval_print(nval* v);
void nval_println(nval* v);
void nval_expr_print(nval* v, char open, char close);
void nval_print_str(nval* v);

/* Code evaluation functions */
nval* nval_eval(nenv* e, nval* v);
nval* nval_eval_sexpr(nenv* e, nval* v);
nval* nval_call(nenv* e, nval* f, nval* a);

#endif