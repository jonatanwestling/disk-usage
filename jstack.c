#include "jstack.h"
#include "mdu.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * @brief Implementation of a stack data structure for the threads to use.
 *
 * The stack is implemented as a linked list with a pointer to the top of the stack.
 * The stack is used to store the tasks that the threads should do.
 *
 * @author Jonatan Westling (c22jwg) with insperation from the stack implementation
 * in the codebase from the doa course.
 * @file jstack.c
 *
 */

struct cell
{
	task_info *value;
	cell *next;
};

struct stack
{
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
stack *stack_empty(void)
{
	stack *s = malloc(sizeof(stack));
	if (s == NULL)
	{
		fprintf(stderr, "stack_empty: memory allocation failed\n");
		exit(EXIT_FAILURE);
	}
	s->top = NULL;
	return s;
}
/**
 * stack_is_empty() - Check if a stack is empty
 *
 * @s: Stack to check
 */
bool stack_is_empty(const stack *s)
{
	return s->top == NULL;
}
/**
 * stack_push() - Push a value onto a stack
 *
 * @s: Stack to push onto
 * @v: task_info to push onto the stack
 *
 * RETURNS: The modified stack
 */
stack *stack_push(stack *s, task_info *v)
{
	cell *e = malloc(sizeof(cell));
	if (e == NULL)
	{
		fprintf(stderr, "stack_push: memory allocation failed\n");
		exit(EXIT_FAILURE);
	}
	e->value = v;
	e->next = s->top;
	s->top = e;
	return s;
}
/**
 * stack_pop() - Pop a task from a stack
 *
 * @s: Stack to pop from, will free the task_info
 * struct at the top of the stack.
 *
 * RETURNS: The modified stack
 */
stack *stack_pop(stack *s)
{
	if (!stack_is_empty(s))
	{
		//relink the stack
		cell *e = s->top;
		s->top = s->top->next;
		//free the task_info struct that was at the top of the stack
		if (e->value != NULL)
		{
			if (e->value->path != NULL)
			{
				free(e->value->path);
			}
			free(e->value);
		}
		free(e);
	}
	return s;
}

/**
 * stack_top() - Get the top value of a stack
 *
 * @s: Stack to get the top value from
 *
 * RETURNS: The top value of the stack
 */
void *stack_top(const stack *s)
{
	if (stack_is_empty(s))
	{
		fprintf(stderr, "stack_top: cannot use on empty stack\n");
		return NULL;
	}
	return s->top->value;
}

/**
 * stack_kill() - Free all memory used by a stack
 *
 * @s: Stack to destroy
 */
void stack_kill(stack *s)
{
	while (!stack_is_empty(s))
	{
		//free all the task_info structs in the stack
		s = stack_pop(s);
	}
	//free the stack itself
	free(s);
}