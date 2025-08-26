#pragma once

#include <stdbool.h>
#include "capy_lexer.h"

typedef struct {
	Token *tokens;
	int current_index;
	int token_length;
} Parser;

typedef enum {
	Variable_INT,
	Variable_FLOAT,
	Variable_STRING,
	Variable_BOOL,
	Variable_MODULE,
} VariableType;

typedef struct Module Module;

typedef struct {
	char *variable_name;
	union {
		char *variable_value;
		Module *module;
	} value;
	VariableType variable_type;
} Variable;

struct Module {
	char *module_name;
	Variable *variables;
	int variable_count;
};

Token peek(Parser *parser);
Token consume(Parser *parser);
bool match(Parser *parser, TokenType token_type);
void parse_init(char *file_path);
void parse_program(char *file_name);
Token *get_tokens(char *src);
void parse_var_declare(Parser *parser);
void parse_assign(Parser *parser, Module *module, char *var_name);
Token parse_expression(Parser *parser, TokenType end_token_type);
void parse_function_call(Parser *parser);