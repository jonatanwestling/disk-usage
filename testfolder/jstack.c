#include "jstack.h"
#include "mdu.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct cell {
    task_info *value;
    cell *next;
};

struct stack {
    cell *top;
};

struct task_info
{
	// path to the dir or file to check and calculate
	char *path;
	// so the thread know which pos to write to when adding the sum to the total sum
	int sum_index;
};
/**
 * stack_empty() - Create an empty stack
 * 
 * RETURNS: An empty stack
 */
stack *stack_empty(void){
    stack *s = malloc(sizeof(stack));
    if (s == NULL) {
        fprintf(stderr, "stack_empty: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    s->top = NULL;
    return s;
}

bool stack_is_empty(const stack *s) {
    return s->top == NULL;
}

stack *stack_push(stack *s, task_info *v) {
    cell *e = malloc(sizeof(cell));
    if (e == NULL) {
        fprintf(stderr, "stack_push: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
   
    e->value = v;
    e->next = s->top;
    s->top = e;
    return s;
}

stack *stack_pop(stack *s) {
    if (!stack_is_empty(s)) {
       cell *e = s->top;
        s->top = s->top->next;
 	if (e->value != NULL) {
            if (e->value->path != NULL) {
               free(e->value->path);
            }
            free(e->value);
        }
        
        free(e);
    }
    return s;
}

void *stack_top(const stack *s) {
    if (stack_is_empty(s)) {
        fprintf(stderr, "stack_top: Warning: top on empty stack\n");
        return NULL; // Modify the behavior if necessary
    }
    return s->top->value;
}

void stack_kill(stack *s) {
    while (!stack_is_empty(s)) {
        s = stack_pop(s);
    }
    free(s);
}