#ifndef _UTILS_H
#define _UTILS_H

#define LENGTH(x) (sizeof(x) / sizeof(*x))
/* #define EXIT_FAILURE  -1 */

typedef enum {
    false,
    true
} bool;

void die(const char *, ...);

#endif
