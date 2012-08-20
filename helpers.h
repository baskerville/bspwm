#ifndef _HELPERS_H
#define _HELPERS_H

#define LENGTH(x)         (sizeof(x) / sizeof(*x))
#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#define MIN(A, B)         ((A) < (B) ? (A) : (B))
#define BOOLSTR(A)        ((A) ? "true" : "false")

#if 1
#  define PUTS(x)            puts(x);
#  define PRINTF(x,...)      printf(x, ##__VA_ARGS__);
#else
#  define PUTS(x)            ;
#  define PRINTF(x)          ;
#endif

typedef enum {
    false,
    true
} bool;

#endif
