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

mpc_parser_t* Number; 
mpc_parser_t* Symbol; 
mpc_parser_t* String; 
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;  
mpc_parser_t* Qexpr;  
mpc_parser_t* Expr; 
mpc_parser_t* Nitrogen;

nval* nval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? nval_num(x) : nval_err("invalid number");
}

nval* nval_read_str(mpc_ast_t* t) {
    t->contents[strlen(t->contents)-1] = '\0';
    char* unescaped = malloc(strlen(t->contents+1)+1);
    strcpy(unescaped, t->contents+1);
    unescaped = mpcf_unescape(unescaped);
    nval* str = nval_str(unescaped);
    free(unescaped);
    return str;
}

nval* nval_read(mpc_ast_t* t) {

    /* If Symbol or Number return conversion to that type */
    if (strstr(t->tag, "number")) { return nval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return nval_sym(t->contents); }
    if (strstr(t->tag, "string")) { return nval_read_str(t); }

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
        if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
        if (strstr(t->children[i]->tag, "comment")) { continue; }
        x = nval_add(x, nval_read(t->children[i]));
    }

    return x;
}

int main(int argc, char** argv) {
    /* Setup MPC parsers */
    Number    = mpc_new("number");
    Symbol    = mpc_new("symbol");
    String    = mpc_new("string");
    Comment   = mpc_new("comment");
    Sexpr     = mpc_new("sexpr");
    Qexpr     = mpc_new("qexpr");
    Expr      = mpc_new("expr");
    Nitrogen  = mpc_new("nitrogen");

    /* Define parsers */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                       \
          number    : /-?[0-9]+/ ;                                              \
          symbol    : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;                        \
          string    : /\"(\\\\.|[^\"])*\"/ ;                                    \
          comment   : /;[^\\r\\n]*/ ;                                           \
          sexpr     : '(' <expr>* ')' ;                                         \
          qexpr     : '{' <expr>* '}' ;                                         \
          expr      : <number> | <symbol> | <sexpr>                             \
                    | <qexpr>  | <string> | <comment> ;                         \
          nitrogen  : /^/ <expr>* /$/ ;                                         \
        ",
        Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Nitrogen);

    nenv* e = nenv_new();
    nenv_add_builtins(e);

    if (argc == 1) {
        puts("Nitrogen Version 1.0.0");
        puts("Press Ctrl+c to Exit\n");

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
    }

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            nval* args = nval_add(nval_sexpr(), nval_str(argv[i]));
            nval* x = builtin_load(e, args);
            if (x->type == NVAL_ERR) { nval_println(x); }
            nval_del(x);
        }
    }

    nenv_del(e);
    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Nitrogen);
    return 0;
}
