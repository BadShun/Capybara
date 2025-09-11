#define _CRT_SECURE_NO_WARNINGS

#include "capy_parser.h"
#include "capy_lexer.h"
#include "capy_loader.h"
#include "capy_stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

int token_count = 0;

Parser *parser;
Module *module;
char *name;

Token peek(Parser *parser) {
	return parser->tokens[parser->current_index];
}

Token consume(Parser *parser) {
	if (parser->current_index >= parser->token_length) {
		Token token = { Token_EOF, NULL, 0 };
		return token;
	}

	return parser->tokens[parser->current_index++];
}

bool match(Parser *parser, TokenType token_type) {
	if (peek(parser).type == token_type) {
		consume(parser);
		return true;
	}

	return false;
}

void parse_init(char *file_name) {
	char *src = read_file(file_name);
	name = (char *)malloc(strlen(file_name) + 1);
	char ch;
	int i = 0;

	while ((ch = file_name[i++]) != '.') {
		name[i - 1] = ch;
	}

	name[i - 1] = '\0';
	name = (char *)realloc(name, i);

	Token *tokens = get_tokens(src);

	parser = (Parser *)malloc(sizeof(Parser));
	parser->tokens = tokens;
	parser->current_index = 0;
	parser->token_length = token_count;

	module = (Module *)malloc(sizeof(Module));
	module->module_name = name;
	module->variables = NULL;
	module->variable_count = 0;
	module->previous_module = NULL;
	module->next_module = NULL;

	/*while (true) {
		Token token = consume(parser);

		print_token(token);

		if (token.type == Token_EOF) {
			break;
		}
	}*/
}

void parse_program(char *file_name) {
	parse_init(file_name);

	parse_statement(parser);
}

void parse_statement(Parser *parser) {
	while (true) {
		Token token = peek(parser);

		if (token.type == Token_EOF) {
			break;
		}

		if (token.type == Token_Keyword) {
			if (strcmp(token.value, "var") == 0) {
				parse_var_declare(parser);
			} else if(strcmp(token.value, "fn") == 0) {
				parse_func_declare(parser);
			}
		} else if (token.type == Token_Identifier) {
			Token name_token = consume(parser);
			token = peek(parser);

			if (token.type == Token_Assign) {
				parse_assign(parser, name_token);
			} else if (token.type == Token_LeftParen) {
				parse_function_call(parser, name_token);
			} else {
				printf("在第%d行发生错误，原因：缺少可修改的左值\n", name_token.line);
				exit(EXIT_FAILURE);
			}

		} else {
			consume(parser);
		}
	}
}

Module * get_current_module() {
	Module *deepest_module = module;

	while (deepest_module->next_module) {
		deepest_module = deepest_module->next_module;
	}

	return deepest_module;
}

bool match_var(char *var_name) {
	Module *current_module = get_current_module();

	while (true) {
		for (int i = 0; i < current_module->variable_count; i++) {
			if (strcmp(var_name, current_module->variables[i].variable_name) == 0) {
				return true;
			}
		}

		if (current_module->previous_module) {
			current_module = current_module->previous_module;
		} else {
			return false;
		}
	}
}

Module * get_matched_moudle(char *var_name) {
	Module *current_module = get_current_module();

	while (true) {
		for (int i = 0; i < current_module->variable_count; i++) {
			if (strcmp(var_name, current_module->variables[i].variable_name) == 0) {
				return current_module;
			}
		}

		if (current_module->previous_module) {
			current_module = current_module->previous_module;
		} else {
			return NULL;
		}
	}
}

VariableType mapping_token_type(TokenType token_type) {
	switch (token_type) {
		case Token_Integer:
			return Variable_INT;
		case Token_Float:
			return Variable_FLOAT;
		case Token_Bool:
			return Variable_BOOL;
		case Token_String:
			return Variable_STRING;
	}
}

void parse_var_declare(Parser *parser) {
	match(parser, Token_Keyword);

	Module *module = get_current_module();
	Variable var;
	Token name_token = peek(parser);
	
	if (match(parser, Token_Identifier)) {
		if (module->variables) {
			for (int i = 0; i < module->variable_count; i++) {
				if (strcmp(module->variables[i].variable_name, name_token.value) == 0) {
					printf("在第%d行发生错误，原因：变量名已存在\n", name_token.line);
					exit(EXIT_FAILURE);
				}
			}
		}

		module->variables = (Variable *)realloc(module->variables, (module->variable_count + 1) * sizeof(Variable));
		var.variable_name = _strdup(name_token.value);
		var.value.variable_value = NULL;
	} else {
		printf("在第%d行发生错误，原因：变量名缺失或非法的标识符%s\n", name_token.line, name_token.value);
		exit(EXIT_FAILURE);
	}

	if (match(parser, Token_Assign)) {
		Token exp_result = parse_expression(parser);
		var.value.variable_value = exp_result.value;
		var.variable_type = mapping_token_type(exp_result.type);
	}

	if (match(parser, Token_Separator)) {
		module->variables[module->variable_count++] = var;
	} else {
		printf("在第%d行发生错误，原因：缺少结束符;\n", name_token.line);
		exit(EXIT_FAILURE);
	}
}

void parse_assign(Parser *parser, Token name_token) {
	match(parser, Token_Assign);

	Module *module = get_current_module();
	Variable var;

	if (!match_var(name_token.value)) {
		printf("在第%d行发生错误，原因：变量%s未声明\n", name_token.line, name_token.value);
		exit(EXIT_FAILURE);
	}

	module = get_matched_moudle(name_token.value);

	for (int i = 0; i < module->variable_count; i++) {
		if (strcmp(module->variables[i].variable_name, name_token.value) == 0) {
			Token exp_result = parse_expression(parser);
			module->variables[i].value.variable_value = exp_result.value;
			module->variables[i].variable_type = mapping_token_type(exp_result.type);
		}
	}
}

void parse_func_declare(Parser *parser) {
	match(parser, Token_Keyword);

	Token name_token = peek(parser);
	Token *arg_tokens = NULL;
	int arg_token_length = 0;
	Token *body_tokens = NULL;
	int body_token_length = 0;

	Module *module = get_current_module();

	if (match(parser, Token_Identifier)) {
		if (match(parser, Token_LeftParen)) {
			Token arg_token = peek(parser);

			while (true) {
				if (arg_token.type == Token_EOF) {
					printf("在第%d行发生错误，原因：缺少结束符)\n", arg_token.line);
					exit(EXIT_FAILURE);
				}
				
				if (arg_token.type == Token_Identifier) {
					arg_tokens = (Token *)realloc(arg_tokens, (arg_token_length + 1) * sizeof(Token));
					arg_tokens[arg_token_length++] = arg_token;
					consume(parser);
					arg_token = peek(parser);
				} else if (arg_token.type == Token_RightParen) {
					break;
				} else {
					printf("在第%d行发生错误，原因：非法的标识符\n", arg_token.line);
					exit(EXIT_FAILURE);
				}
				
				if (arg_token.type == Token_Comma) {
					consume(parser);
					arg_token = peek(parser);
				} else if (arg_token.type == Token_RightParen) {
					break;
				} else {
					printf("在第%d行发生错误，原因：缺少结束符,或)\n", arg_token.line);
					exit(EXIT_FAILURE);
				}
			}

			match(parser, Token_RightParen);

			if (match(parser, Token_LeftBrace)) {
				int brace_count = 1;

				while (brace_count) {
					Token body_token = peek(parser);

					if (body_token.type == Token_EOF) {
						printf("在第%d行发生错误，原因：缺少符号}\n", arg_token.line);
						exit(EXIT_FAILURE);
					}

					if (body_token.type == Token_LeftBrace) {
						brace_count++;
					} else if (body_token.type == Token_RightBrace) {
						brace_count--;
					}

					if (brace_count) {
						body_tokens = (Token *)realloc(body_tokens, (body_token_length + 1) * sizeof(Token));
						body_tokens[body_token_length++] = body_token;
						consume(parser);
					} else {
						match(parser, Token_RightBrace);
					}
				}


				Token func_end_token = { Token_EOF, NULL, body_tokens[body_token_length - 1].line };
				body_tokens = (Token *)realloc(body_tokens, (body_token_length + 1) * sizeof(Token));
				body_tokens[body_token_length++] = func_end_token;
			} else {
				printf("在第%d行发生错误，原因：缺少符号{\n", arg_token.line, arg_token.value);
				exit(EXIT_FAILURE);
			}
		}

		int index = 0;

		Variable func_var;
		func_var.variable_type = Variable_FUNC;
		func_var.variable_name = name_token.value;

		Function *function = malloc(sizeof(Function));
		function->arg_tokens = arg_tokens;
		function->arg_token_length = arg_token_length;
		function->body_tokens = body_tokens;
		function->body_token_length = body_token_length;

		func_var.value.function = function;

		for (int i = 0; i < module->variable_count; i++) {
			if (strcmp(name_token.value, module->variables[i].variable_name) == 0) {
				func_var.variable_name = module->variables[i].variable_name;
				index = i;
			}
		}

		if (index) {
			module->variables[index] = func_var;
		} else {
			module->variables = (Variable *)realloc(module->variables, (module->variable_count + 1) * sizeof(Variable));
			module->variables[module->variable_count++] = func_var;
		}
	} else {
		printf("在第%d行发生错误，原因：非法的标识符%s\n", name_token.line, name_token.value);
		exit(EXIT_FAILURE);
	}
}

void parse_function_call(Parser *parser, Token name_token) {
	match(parser, Token_LeftParen);

	Module *module = get_current_module();
	Function *func = NULL;

	for (int i = 0; i < module->variable_count; i++) {
		if (strcmp(name_token.value, module->variables[i].variable_name) == 0 && module->variables[i].variable_type == Variable_FUNC) {
			func = module->variables[i].value.function;
		}
	}

	if (func) {
		Parser *func_parser = (Parser *)malloc(sizeof(Parser));
		func_parser->tokens = func->body_tokens;
		func_parser->token_length = func->body_token_length;
		func_parser->current_index = 0;

		Token *arg_tokens = NULL;
		int arg_token_length = 0;

		if (match(parser, Token_RightParen)) {
		
		} else {
			while (true) {
				Token arg_token = parse_expression(parser);
				arg_tokens = (Token *)realloc(arg_tokens, (arg_token_length + 1) * sizeof(Token));
				arg_tokens[arg_token_length++] = arg_token;

				if (match(parser, Token_Comma)) {

				} else if (match(parser, Token_RightParen)) {
					break;
				}
			}
		}

		Module *func_module = (Module *)malloc(sizeof(Module));
		func_module->variable_count = 0;
		func_module->variables = NULL;
		func_module->previous_module = get_current_module();
		func_module->next_module = NULL;

		if (arg_token_length == func->arg_token_length) {
			for (int i = 0; i < arg_token_length; i++) {
				Variable var;
				var.value.variable_value = arg_tokens[i].value;
				var.variable_name = func->arg_tokens[i].value;
				var.variable_type = mapping_token_type(arg_tokens[i].type);

				func_module->variables = (Variable *)realloc(func_module->variables, (func_module->variable_count + 1) * sizeof(Variable));
				func_module->variables[func_module->variable_count++] = var;
			}
		} else {
			printf("在第%d行发生错误，原因：函数参数数量不匹配\n", name_token.line);
			exit(EXIT_FAILURE);
		}

		get_current_module()->next_module = func_module;
		parse_statement(func_parser);
		get_current_module()->previous_module->next_module = NULL;
		match(parser, Token_Separator);
	} else {
		printf("在第%d行发生错误，原因：函数%s未定义\n", name_token.line, name_token.value);
		exit(EXIT_FAILURE);
	}
}

int get_priority(Token op_token) {
	switch (op_token.type) {
		case Token_Keyword:
			if (strcmp(op_token.value, "or") == 0) {
				return 1;
			} else if (strcmp(op_token.value, "and") == 0) {
				return 2;
			} else {
				printf("在第%d行发生错误，原因：未知操作符\n", op_token.line);
				exit(EXIT_FAILURE);
			}
		case Token_Equal:
		case Token_NotEqual:
			return 3;
		case Token_GreaterThan:
		case Token_GreaterThanEqual:
		case Token_LessThan:
		case Token_LessThanEqual:
			return 4;
		case Token_Add:
		case Token_Subtract:
			return 5;
		case Token_Multiply:
		case Token_Divide:
			return 6;
		case Token_Not:
			return 7;
		case Token_Positive:
		case Token_Negative:
			return 8;
		default:
			printf("在第%d行发生错误，原因：未知操作符\n", op_token.line);
			exit(EXIT_FAILURE);
	}
}

Token exp_add(Token left_value_token, Token right_value_token) {
	Token result_token;

	if (left_value_token.type == Token_String || right_value_token.type == Token_String) {
		char *result = (char *)malloc(strlen(left_value_token.value) + strlen(right_value_token.value));
		strcpy(result, left_value_token.value);
		strcat(result, right_value_token.value);

		result_token.type = Token_String;
		result_token.value = result;
	} else if (left_value_token.type == Token_Float || right_value_token.type == Token_Float) {
		double left_value = atof(left_value_token.value);
		double right_value = atof(right_value_token.value);

		double result_double = left_value + right_value;
		int result_len = snprintf(NULL, 0, "%lf", result_double);
		char *result_string = (char *)malloc(result_len + 1);
		snprintf(result_string, result_len + 1, "%lf", result_double);

		result_token.type = Token_Float;
		result_token.value = result_string;
	} else {
		int left_value = atoi(left_value_token.value);
		int right_value = atoi(right_value_token.value);

		int result_integer = left_value + right_value;
		int result_len = snprintf(NULL, 0, "%d", result_integer);
		char *result_string = (char *)malloc(result_len + 1);
		snprintf(result_string, result_len + 1, "%d", result_integer);

		result_token.type = Token_Integer;
		result_token.value = result_string;
	}

	return result_token;
}

Token exp_subtract(Token left_value_token, Token right_value_token) {
	Token result_token;

	if (left_value_token.type == Token_String || right_value_token.type == Token_String) {
		printf("在第%d行发生错误，原因：字符串不支持减法运算\n", left_value_token.line);
		exit(EXIT_FAILURE);
	} else if (left_value_token.type == Token_Float || right_value_token.type == Token_Float) {
		double left_value = atof(left_value_token.value);
		double right_value = atof(right_value_token.value);

		double result_double = left_value - right_value;
		int result_len = snprintf(NULL, 0, "%lf", result_double);
		char *result_string = (char *)malloc(result_len + 1);
		snprintf(result_string, result_len + 1, "%lf", result_double);

		result_token.type = Token_Float;
		result_token.value = result_string;
	} else {
		int left_value = atoi(left_value_token.value);
		int right_value = atoi(right_value_token.value);

		int result_integer = left_value - right_value;
		int result_len = snprintf(NULL, 0, "%d", result_integer);
		char *result_string = (char *)malloc(result_len + 1);
		snprintf(result_string, result_len + 1, "%d", result_integer);

		result_token.type = Token_Integer;
		result_token.value = result_string;
	}

	return result_token;
}

Token exp_multiply(Token left_value_token, Token right_value_token) {
	Token result_token;

	if (left_value_token.type == Token_String || right_value_token.type == Token_String) {
		printf("在第%d行发生错误，原因：字符串不支持乘法运算\n", left_value_token.line);
		exit(EXIT_FAILURE);
	} else if (left_value_token.type == Token_Float || right_value_token.type == Token_Float) {
		double left_value = atof(left_value_token.value);
		double right_value = atof(right_value_token.value);

		double result_double = left_value * right_value;
		int result_len = snprintf(NULL, 0, "%lf", result_double);
		char *result_string = (char *)malloc(result_len + 1);
		snprintf(result_string, result_len + 1, "%lf", result_double);

		result_token.type = Token_Float;
		result_token.value = result_string;
	} else {
		int left_value = atoi(left_value_token.value);
		int right_value = atoi(right_value_token.value);

		int result_integer = left_value * right_value;
		int result_len = snprintf(NULL, 0, "%d", result_integer);
		char *result_string = (char *)malloc(result_len + 1);
		snprintf(result_string, result_len + 1, "%d", result_integer);

		result_token.type = Token_Integer;
		result_token.value = result_string;
	}

	return result_token;
}

Token exp_divide(Token left_value_token, Token right_value_token) {
	Token result_token;

	if (left_value_token.type == Token_String || right_value_token.type == Token_String) {
		printf("在第%d行发生错误，原因：字符串不支持除法运算\n", left_value_token.line);
		exit(EXIT_FAILURE);
	}

	double eps = 1e-9;
	if (atof(right_value_token.value) < eps) {
		printf("在第%d行发生错误，原因：不可以除以0\n", left_value_token.line);
		exit(EXIT_FAILURE);
	}

	double left_value = atof(left_value_token.value);
	double right_value = atof(right_value_token.value);

	double result_double = left_value / right_value;
	int result_len = snprintf(NULL, 0, "%lf", result_double);
	char *result_string = (char *)malloc(result_len + 1);
	snprintf(result_string, result_len + 1, "%lf", result_double);

	result_token.type = Token_Float;
	result_token.value = result_string;

	return result_token;
}

Token exp_equal(Token left_value_token, Token right_value_token) {
	Token result_token;
	result_token.type = Token_Bool;

	if (left_value_token.type == Token_String || right_value_token.type == Token_String) {
		result_token.value = strcmp(left_value_token.value, right_value_token.value) == 0 ? "true" : "false";
	} else {
		double left_value = atof(left_value_token.value);
		double right_value = atof(right_value_token.value);
		double eps = 1e-9;

		result_token.value = fabs(left_value - right_value) < eps ? "true" : "false";
	}

	return result_token;
}

Token exp_not_equal(Token left_value_token, Token right_value_token) {
	Token result_token = exp_equal(left_value_token, right_value_token);
	result_token.value = strcmp(result_token.value, "true") == 0 ? "false" : "true";

	return result_token;
}

Token exp_or(Token left_value_token, Token right_value_token) {
	Token result_token;
	result_token.type = Token_Bool;

	if (left_value_token.type == Token_String || right_value_token.type == Token_String) {
		result_token.value = "true";
	} else if (left_value_token.type == Token_Bool && strcmp(left_value_token.value, "true") == 0) {
		result_token.value = "true";
	} else if (right_value_token.type == Token_Bool && strcmp(right_value_token.value, "true") == 0) {
		result_token.value = "true";
	} else {
		double left_value = atof(left_value_token.value);
		double right_value = atof(right_value_token.value);
		double eps = 1e-9;
		
		result_token.value = (left_value > eps || right_value > eps) ? "true" : "false";
	}

	return result_token;
}

Token exp_and(Token left_value_token, Token right_value_token) {
	Token result_token;
	result_token.type = Token_Bool;

	bool left_value_bool;
	bool right_value_bool;

	if (left_value_token.type == Token_String) {
		left_value_bool = true;
	} else if (left_value_token.type == Token_Bool) {
		if (strcmp(left_value_token.value, "true") == 0) {
			left_value_bool = true;
		} else {
			left_value_bool = false;
		}
	} else {
		double left_value = atof(left_value_token.value);
		double eps = 1e-9;

		left_value_bool = left_value > eps;
	}

	if (right_value_token.type == Token_String) {
		right_value_bool = true;
	} else if (right_value_token.type == Token_Bool) {
		if (strcmp(right_value_token.value, "true") == 0) {
			right_value_bool = true;
		} else {
			right_value_bool = false;
		}
	} else {
		double right_value = atof(right_value_token.value);
		double eps = 1e-9;

		right_value_bool = right_value > eps;
	}

	if (left_value_bool && right_value_bool) {
		result_token.value = "true";
	} else {
		result_token.value = "false";
	}

	return result_token;
}

Token exp_greater_than(Token left_value_token, Token right_value_token) {
	Token result_token;
	result_token.type = Token_Bool;

	if (left_value_token.type == Token_String || right_value_token.type == Token_String) {
		printf("在第%d行发生错误，原因：字符串不支持大于运算\n", left_value_token.line);
		exit(EXIT_FAILURE);
	} else {
		double left_value = atof(left_value_token.value);
		double right_value = atof(right_value_token.value);

		result_token.value = left_value > right_value ? "true" : "false";
	}

	return result_token;
}

Token exp_less_than(Token left_value_token, Token right_value_token) {
	Token result_token;
	result_token.type = Token_Bool;

	if (left_value_token.type == Token_String || right_value_token.type == Token_String) {
		printf("在第%d行发生错误，原因：字符串不支持小于运算\n", left_value_token.line);
		exit(EXIT_FAILURE);
	} else {
		double left_value = atof(left_value_token.value);
		double right_value = atof(right_value_token.value);

		result_token.value = left_value < right_value ? "true" : "false";
	}

	return result_token;
}

Token exp_greater_than_equal(Token left_value_token, Token right_value_token) {
	Token result_token;
	result_token.type = Token_Bool;
	Token greater_than_result_token = exp_greater_than(left_value_token, right_value_token);

	if (strcmp(greater_than_result_token.value, "true") == 0) {
		result_token.value = "true";
	} else {
		Token equal_result_token = exp_equal(left_value_token, right_value_token);
		result_token.value = equal_result_token.value;
	}

	return result_token;
}

Token exp_less_than_equal(Token left_value_token, Token right_value_token) {
	Token result_token;
	result_token.type = Token_Bool;
	Token less_than_result_token = exp_less_than(left_value_token, right_value_token);

	if (strcmp(less_than_result_token.value, "true") == 0) {	
		result_token.value = "true";
	} else {
		Token equal_result_token = exp_equal(left_value_token, right_value_token);
		result_token.value = equal_result_token.value;
	}

	return result_token;
}

Token exp_not(Token value_token) {
	Token result_token;
	result_token.type = Token_Bool;

	if (value_token.type == Token_String) {
		result_token.value = "false";
	} else if (value_token.type == Token_Bool) {
		result_token.value = strcmp(result_token.value, "true") == 0 ? "false" : "true";
	} else {
		double value = atof(value_token.value);
		double eps = 1e-9;

		if (value > eps) {
			result_token.value = "false";
		} else {
			result_token.value = "true";
		}
	}

	return result_token;
}

Token exp_positive(Token value_token) {
	Token result_token;

	if (value_token.type == Token_String) {
		printf("在第%d行发生错误，原因：字符串不支持取正\n", value_token.line);
		exit(EXIT_FAILURE);
	} else {
		char *result = (char *)malloc(strlen(value_token.value) + 1);
		strcpy(result, "+");
		strcat(result, value_token.value);

		result_token.line = value_token.line;
		result_token.type = value_token.type;
		result_token.value = result;
	}

	return result_token;
}

Token exp_negative(Token value_token) {
	Token result_token;

	if (value_token.type == Token_String) {
		printf("在第%d行发生错误，原因：字符串不支持取负\n", value_token.line);
		exit(EXIT_FAILURE);
	} else {
		char *result = (char *)malloc(strlen(value_token.value) + 1);
		strcpy(result, "-");
		strcat(result, value_token.value);

		result_token.line = value_token.line;
		result_token.type = value_token.type;
		result_token.value = result;
	}

	return result_token;
}

Token parse_expression(Parser *parser) {
	Token *exp_buffer = NULL;
	Token exp_result_token;
	TokenType end_token_types[] = { Token_Separator, Token_RightParen, Token_Comma };
	int end_token_types_len = sizeof(end_token_types) / sizeof(TokenType);
	memset(&exp_result_token, 0, sizeof(Token));
	int exp_buffer_length = 0;

	Module *module = get_current_module();

	while (true) {
		Token token = peek(parser);

		if (token.type == Token_EOF) {
			printf("在第%d行发生错误，原因：缺少表达式结束符\n", token.line);
			exit(EXIT_FAILURE);
		}

		bool exp_end_flag = false;

		for (int i = 0; i < end_token_types_len; i++) {
			if (token.type == end_token_types[i]) {
				exp_end_flag = true;
			}
		}

		if (exp_end_flag) {
			break;
		}

		exp_buffer = (Token *)realloc(exp_buffer, (++exp_buffer_length) * sizeof(Token));

		if (match(parser, Token_Identifier)) {
			if (match(parser, Token_LeftParen)) {
				// 调用函数
			} else if (match(parser, Token_Dot)) {

			} else {
				if (!match_var(token.value)) {
					printf("在第%d行发生错误，原因：使用了未声明的变量%s", token.line, token.value);
					exit(EXIT_FAILURE);
				}

				module = get_matched_moudle(token.value);

				for (int i = 0; i < module->variable_count; i++) {
					if (strcmp(module->variables[i].variable_name, token.value) == 0) {
						Variable var = module->variables[i];

						if (var.value.variable_value) {
							token.value = var.value.variable_value;

							switch (var.variable_type) {
								case Variable_INT:
									token.type = Token_Integer;
									break;
								case Variable_FLOAT:
									token.type = Token_Float;
									break;
								case Variable_BOOL:
									token.type = Token_Bool;
									break;
								case Variable_STRING:
									token.type = Token_String;
									break;
							}

							exp_buffer[exp_buffer_length - 1] = token;
						} else {
							printf("在第%d行发生错误，原因：使用了未定义的变量%s", token.line, token.value);
							exit(EXIT_FAILURE);
						}
					}
				}
			}
		} else if (match(parser, Token_Bool)) {
			if (strcmp(token.value, "true") == 0) {
				Token bool_token = { Token_Integer, "1", token.line };
				exp_buffer[exp_buffer_length - 1] = bool_token;
			} else if (strcmp(token.value, "false") == 0) {
				Token bool_token = { Token_Integer, "0", token.line };
				exp_buffer[exp_buffer_length - 1] = bool_token;
			}
		} else {
			if (token.type == Token_Add) {
				if (exp_buffer_length == 1) {
					token.type = Token_Positive;
					exp_buffer[exp_buffer_length - 1] = token;
					consume(parser);
				} else {
					Token previous_token = exp_buffer[exp_buffer_length - 2];
					switch (previous_token.type) {
						case Token_Integer:
						case Token_Float:
						case Token_String:
							exp_buffer[exp_buffer_length - 1] = token;
							consume(parser);
							break;
						default:
							token.type = Token_Positive;
							exp_buffer[exp_buffer_length - 1] = token;
							consume(parser);
					}
				}
			} else if (token.type == Token_Subtract) {
				if (exp_buffer_length == 1) {
					token.type = Token_Negative;
					exp_buffer[exp_buffer_length - 1] = token;
					consume(parser);
				} else {
					Token previous_token = exp_buffer[exp_buffer_length - 2];
					switch (previous_token.type) {
						case Token_Integer:
						case Token_Float:
						case Token_String:
							exp_buffer[exp_buffer_length - 1] = token;
							consume(parser);
							break;
						default:
							token.type = Token_Negative;
							exp_buffer[exp_buffer_length - 1] = token;
							consume(parser);
					}
				}
			} else if (match(parser, Token_LeftParen)) { // 处理括号
				exp_buffer[exp_buffer_length - 1] = parse_expression(parser);
				match(parser, Token_RightParen);
			} else {
				exp_buffer[exp_buffer_length - 1] = token;
				consume(parser);
			}
		}
	}

	Token *postfix_exp_buffer = NULL;
	int postfix_exp_buffer_length = 0;
	Stack *op_stack = (Stack *)malloc(sizeof(Stack));
	stack_init(op_stack);

	for (int i = 0; i < exp_buffer_length; i++) {
		Token token = exp_buffer[i];

		switch (token.type) {
			case Token_Integer:
			case Token_Float:
			case Token_Bool:
			case Token_String:
				postfix_exp_buffer = (Token *)realloc(postfix_exp_buffer, (++postfix_exp_buffer_length) * sizeof(Token));
				postfix_exp_buffer[postfix_exp_buffer_length - 1] = token;
				continue;
		}

		if (is_empty(op_stack)) {
			push(op_stack, token);
		} else {
			if (get_priority(token) > get_priority(peek_top(op_stack))) {
				push(op_stack, token);
			} else {
				while (true) {
					postfix_exp_buffer = (Token *)realloc(postfix_exp_buffer, (++postfix_exp_buffer_length) * sizeof(Token));
					postfix_exp_buffer[postfix_exp_buffer_length - 1] = pop(op_stack);

					if (is_empty(op_stack) || (get_priority(token) > get_priority(peek_top(op_stack)))) {
						push(op_stack, token);
						break;
					}
				}
			}
		}
	}

	while (!is_empty(op_stack)) {
		postfix_exp_buffer = (Token *)realloc(postfix_exp_buffer, (++postfix_exp_buffer_length) * sizeof(Token));
		postfix_exp_buffer[postfix_exp_buffer_length - 1] = pop(op_stack);
	}

	Stack *value_stack = (Stack *)malloc(sizeof(Stack));
	stack_init(value_stack);

	for (int i = 0; i < postfix_exp_buffer_length; i++) {
		Token exp_token = postfix_exp_buffer[i];
		Token left_value_token;
		Token right_value_token;

		switch (exp_token.type) {
			case Token_Integer:
			case Token_Float:
			case Token_Bool:
			case Token_String:
				push(value_stack, exp_token);
				continue;
			case Token_Add: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_add(left_value_token, right_value_token));
				break;
			}
			case Token_Subtract: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_subtract(left_value_token, right_value_token));
				break;
			}
			case Token_Multiply: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_multiply(left_value_token, right_value_token));
				break;
			}
			case Token_Divide: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_divide(left_value_token, right_value_token));
				break;
			}
			case Token_Equal: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_equal(left_value_token, right_value_token));
				break;
			}
			case Token_NotEqual: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_not_equal(left_value_token, right_value_token));
				break;
			}
			case Token_Or: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_or(left_value_token, right_value_token));
				break;
			}
			case Token_And: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_and(left_value_token, right_value_token));
				break;
			}
			case Token_GreaterThan: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_greater_than(left_value_token, right_value_token));
				break;
			}
			case Token_LessThan: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_less_than(left_value_token, right_value_token));
				break;
			}
			case Token_GreaterThanEqual: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_greater_than_equal(left_value_token, right_value_token));
				break;
			}
			case Token_LessThanEqual: {
				right_value_token = pop(value_stack);
				left_value_token = pop(value_stack);
				push(value_stack, exp_less_than_equal(left_value_token, right_value_token));
				break;
			}
			case Token_Positive: {
				Token value_token = pop(value_stack);
				push(value_stack, exp_positive(value_token));
				break;
			}
			case Token_Negative: {
				Token value_token = pop(value_stack);
				push(value_stack, exp_negative(value_token));
				break;
			}
			case Token_Not: {
				Token value_token = pop(value_stack);
				push(value_stack, exp_not(value_token));
				break;
			}
		}
	}

	exp_result_token = pop(value_stack);
	print_token(exp_result_token);

	return exp_result_token;
}

Token * get_tokens(char *src) {
	lexer_init(src);

	int token_buffer_size = 30;
	Token *tokens = (Token *)malloc(token_buffer_size * sizeof(Token));

	while (true) {
		if (token_count >= token_buffer_size) {
			token_buffer_size *= 2;
			tokens = (Token *)realloc(tokens, token_buffer_size * sizeof(Token));
		}

		tokens[token_count++] = next_token();

		if (tokens[token_count-1].type == Token_EOF) {
			break;
		}
	}

	tokens = (Token *)realloc(tokens, token_count * sizeof(Token));
	return tokens;
}
