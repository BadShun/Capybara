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
	Variable_FUNC
} VariableType;

typedef enum {
	Statement_GLOBAL = 1 << 0,
	Statement_FUNC   = 1 << 1,
	Statement_IF     = 1 << 2,
	Statement_WHILE  = 1 << 3,
} StatementType;

typedef struct Module Module;
typedef struct Function Function;

typedef struct {
	char *variable_name;
	union {
		char *variable_value;
		Module *module;
		Function *function;
	} value;
	VariableType variable_type;
} Variable;

struct Module {
	char *module_name;
	Variable *variables;
	int variable_count;
	Module *previous_module;
	Module *next_module;
};

struct Function {
	Token *arg_tokens;
	int arg_token_length;
	Token *body_tokens;
	int body_token_length;
};

Token peek(Parser *parser);
Token consume(Parser *parser);
bool match(Parser *parser, TokenType token_type);
void parse_init(char *file_path);
void parse_program(char *file_name);
Module *get_current_module();
bool match_var(char *var_name);
Module *get_matched_module(char *var_name);
Token *get_tokens(char *src);
void parse_var_declare(Parser *parser);
void parse_assign(Parser *parser, Token name_token);
void parse_func_declare(Parser *parser);
void parse_if(Parser *parser);
void parse_while(Parser *parser);
Token parse_function_call(Parser *parser, Token name_token);
Token parse_statement(Parser *parser, int statement_type);
Token parse_expression(Parser *parser);
