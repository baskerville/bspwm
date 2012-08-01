#ifndef _UTILS_H
#define _UTILS_H

#define LENGTH(x)  (sizeof(x) / sizeof(*x))
#define MAX(A, B)  ((A) > (B) ? (A) : (B))
#define MIN(A, B)  ((A) < (B) ? (A) : (B))

typedef enum {
    false,
    true
} bool;

void die(const char *, ...);

#endif
