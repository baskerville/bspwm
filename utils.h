#define LENGTH(x) (sizeof(x) / sizeof(*x))
/* #define EXIT_FAILURE  -1 */

typedef enum {
    false,
    true
} bool;

void die(const char *errstr, ...);
