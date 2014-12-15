#include <stdio.h>
#include "nstdlib.h"
#include "ncore.h"

/* Simple math functions */
long min(long x, long y) {
    if (x < y) {
        return x;
    }
    return y;
}

long max(long x, long y) {
    if (x > y) {
        return x;
    }
    return y;
}