#ifndef _STACK_H
#define _STACK_H

stacking_list_t *make_stack(node_t *n);
void stack_insert_after(stacking_list_t *a, node_t *n);
void stack_insert_before(stacking_list_t *a, node_t *n);
void remove_stack(stacking_list_t *s);
void remove_stack_node(node_t *n);
void stack(node_t *n);
void stack_under(node_t *n);

#endif
