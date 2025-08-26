#include "capy_stack.h"
#include "capy_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void stack_init(Stack *stack) {
	stack->top = NULL;
}

bool is_empty(Stack *stack) {
	return stack->top == NULL;
}

void push(Stack *stack, Token value) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->data = value;
    new_node->next = stack->top;
    stack->top = new_node;
}

Token pop(Stack *stack) {
    if (is_empty(stack)) {
        printf("Õ»Îª¿Õ\n");
        exit(EXIT_FAILURE);
    }

    Node *temp = stack->top;
    Token value = temp->data;
    stack->top = temp->next;
    free(temp);
    return value;
}

Token peek_top(Stack *stack) {
    if (is_empty(stack)) {
        printf("Õ»Îª¿Õ\n");
        exit(EXIT_FAILURE);
    }

    return stack->top->data;
}

void free_stack(Stack *stack) {
    while (!is_empty(stack)) {
        pop(stack);
    }
}
