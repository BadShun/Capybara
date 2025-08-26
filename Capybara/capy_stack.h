#pragma once

#include <stdbool.h>
#include "capy_lexer.h"

typedef struct Node {
	Token data;
	struct Node *next;
} Node;

typedef struct {
	Node *top;
} Stack;

void stack_init(Stack *stack);
bool is_empty(Stack *stack);
void push(Stack *stack, Token value);
Token pop(Stack *stack);
Token peek_top(Stack *stack);