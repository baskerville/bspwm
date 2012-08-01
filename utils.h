#ifndef _UTILS_H
#define _UTILS_H

#define LENGTH(x)         (sizeof(x) / sizeof(*x))
#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#define MIN(A, B)         ((A) < (B) ? (A) : (B))
#define RESPONSE_TYPE(e)  (e->response_type & ~0x80)

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

void die(const char *, ...);

#endif
