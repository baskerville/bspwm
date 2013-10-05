#ifndef _STACK_H
#define _STACK_H

stack_t *make_stack(node_t *n);
void stack_insert_after(stack_t *a, node_t *n);
void stack_insert_before(stack_t *a, node_t *n);
void remove_stack(stack_t *s);
void remove_stack_node(node_t *n);
void stack(node_t *n);
void stack_under(node_t *n);

#endif
