#ifndef ncore
#define ncore

struct nval;
struct nenv;
typedef struct nval nval;
typedef struct nenv nenv;

enum { NVAL_NUM, NVAL_ERR, NVAL_SYM, NVAL_SEXPR, NVAL_QEXPR, NVAL_FUN };

typedef nval*(*nbuiltin)(nenv*, nval*);

struct nval {
    int type;

    long num;
    char* err;
    char* sym;

    nbuiltin builtin;
    nenv* env;
    nval* formals;
    nval* body;

    int count;
    nval** cell;
};

struct nenv {
    nenv* par;
    int count;
    char** syms;
    nval** vals;
};

/* Constuctor and destructor for environment types */
nenv* nenv_new(void);
void nenv_del(nenv* e);

/* environment manipulation functions */
nval* nenv_get(nenv* e, nval* k);
void nenv_put(nenv* e, nval* k, nval* v);
void nenv_rem(nenv* e, nval* k);
nenv* nenv_copy(nenv* e);
void nenv_def(nenv* e, nval* k, nval* v);

/* Constructor functions for nval types */
nval* nval_num(long x);
nval* nval_err(char* fmt, ...);
nval* nval_sym(char* s);
nval* nval_sexpr(void);
nval* nval_qexpr(void);
nval* nval_fun(nbuiltin func);
nval* nval_lambda(nval* formals, nval* body);

/* nval manipulation functions */
void nval_del(nval* v);
nval* nval_add(nval* v, nval* x);
nval* nval_pop(nval* v, int i);
nval* nval_take(nval* v, int i);
nval* nval_join(nval* x, nval* y);
nval* nval_copy(nval* v);
char* ntype_name(int t);

/* Builtin functions and operations */
void nenv_add_builtin(nenv *e, char* name, nbuiltin func);
void nenv_add_builtins(nenv* e);
/* Arithmatic operations */
nval* builtin_op(nenv* e, nval* a, char* op);
nval* builtin_add(nenv* e, nval* a);
nval* builtin_sub(nenv* e, nval* a);
nval* builtin_mul(nenv* e, nval* a);
nval* builtin_div(nenv* e, nval* a);

/* Q-Expression functions */
nval* builtin_head(nenv* e, nval* a);
nval* builtin_tail(nenv* e, nval* a);
nval* builtin_list(nenv* e, nval* a);
nval* builtin_eval(nenv* e, nval* a);
nval* builtin_join(nenv* e, nval* a);

nval* builtin_def(nenv* e, nval* a);
nval* builtin_put(nenv* e, nval* a);
nval* builtin_var(nenv* e, nval* a, char* func);
nval* builtin_undef(nenv* e, nval* a);
nval* builtin_lambda(nenv* e, nval* a);

/* Core print statements */
void nval_print(nval* v);
void nval_println(nval* v);
void nval_expr_print(nval* v, char open, char close);

/* Code evaluation functions */
nval* nval_eval(nenv* e, nval* v);
nval* nval_eval_sexpr(nenv* e, nval* v);
nval* nval_call(nenv* e, nval* f, nval* a);

#endif