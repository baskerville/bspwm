#ifndef _STACK_H
#define _STACK_H

stack_t *make_stack(node_t *);
void stack_insert_after(stack_t *, node_t *);
void stack_insert_before(stack_t *, node_t *);
void remove_stack(stack_t *);
void remove_stack_node(node_t *);
void stack(node_t *);
void stack_under(node_t *);

#endif
