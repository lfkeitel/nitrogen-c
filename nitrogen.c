#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mpc.h"

#include "ncore.h"
#include "builtins.h"

/* Windows doesn't use the editline library */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

nval* nval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? nval_num(x) : nval_err("invalid number");
}

nval* nval_read(mpc_ast_t* t) {

    /* If Symbol or Number return conversion to that type */
    if (strstr(t->tag, "number")) { return nval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return nval_sym(t->contents); }

    /* If root (>) or s/qexpr then create empty list */
    nval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = nval_sexpr(); } 
    if (strstr(t->tag, "sexpr"))  { x = nval_sexpr(); }
    if (strstr(t->tag, "qexpr"))  { x = nval_qexpr(); }

    /* Fill this list with any valid expression contained within */
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        x = nval_add(x, nval_read(t->children[i]));
    }

    return x;
}

int main(int argc, char** argv) {
    /* Setup MPC parsers */
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Symbol    = mpc_new("symbol");
    mpc_parser_t* Sexpr     = mpc_new("sexpr");
    mpc_parser_t* Qexpr     = mpc_new("qexpr");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Nitrogen  = mpc_new("nitrogen");

    /* Define parsers */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                       \
          number    : /-?[0-9]+/ ;                                              \
          symbol    : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;                        \
          sexpr     : '(' <expr>* ')' ;                                         \
          qexpr     : '{' <expr>* '}' ;                                         \
          expr      : <number> | <symbol> | <sexpr> | <qexpr> ;                 \
          nitrogen  : /^/ <expr>* /$/ ;                                         \
        ",
        Number, Symbol, Sexpr, Qexpr, Expr, Nitrogen);

    puts("Nitrogen Version 0.0.2");
    puts("Press Ctrl+c to Exit\n");

    nenv* e = nenv_new();
    nenv_add_builtins(e);

    while (1) {
        char* input = readline("nitrogen> ");
        add_history(input);
        
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Nitrogen, &r)) {
            nval* x = nval_eval(e, nval_read(r.output));
            nval_println(x);
            nval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    nenv_del(e);
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Nitrogen);
    return 0;
}
