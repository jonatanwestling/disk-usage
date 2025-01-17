#ifndef JSTACK_H
#define JSTACK_H

#include <stdbool.h>
#include "mdu.h"

// ==========PUBLIC DATA TYPES============

// Stack type.
typedef struct stack stack;
typedef struct cell cell;


// ==========DATA STRUCTURE INTERFACE==========

stack *stack_empty(void);

bool stack_is_empty(const stack *s);

stack *stack_push(stack *s, task_info *task);

stack *stack_pop(stack *s);

void *stack_top(const stack *s);

void stack_kill(stack *s);


#endif
