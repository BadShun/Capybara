#define _CRT_SECURE_NO_WARNINGS

#include "capy_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

const char *keywords[] = { "var", "true", "false", "and", "or", "if", "elif", "else", "while", "fn", "return" };
const int keywords_count = sizeof(keywords) / sizeof(char *);

char *src;
int current_line = 1;

void lexer_init(char *content) {
	src = content;
}

char next() {
	return *(src++);
}

Token next_token() {
	// token缓冲区
	char buffer[1024];
	// 跳过空白字符
	while (*src == ' ' || *src == '\n' || *src == '\t') {
		if (*src == '\n') {
			current_line++;
		}

		next();
	}
	// 跳过注释
	if (*src == '#') {
		next();

		while (true) {
			if (*src == '\n') {
				current_line++;
				next();
				break;
			} else if (*src == '\0') {
				Token token = { Token_EOF, NULL, current_line };
				return token;
			} else {
				next();
			}
		}

		return next_token();
	}
	// 生成关键字和标识符token
	if ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || *src == '_') {
		int i = 0;
		buffer[i++] = next();

		while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || *src == '_') {
			buffer[i++] = next();
		}

		buffer[i] = '\0';
		char *token_value = (char *)malloc((i + 1) * sizeof(char));
		strcpy(token_value, buffer);

		Token token;

		if (strcmp(token_value, "true") == 0 || strcmp(token_value, "false") == 0) {
			token.type = Token_Bool;
		} else {
			token.type = is_keyword(token_value) ? Token_Keyword : Token_Identifier;
		}

		token.value = token_value;
		token.line = current_line;

		return token;
	}
	// 生成数字token（包含小数）
	if (*src >= '0' && *src <= '9') {
		int i = 0;
		bool is_float = false;
		buffer[i++] = next();

		while ((*src >= '0' && *src <= '9') || *src == '.') {
			if (*src == '.' && !is_float) {
				is_float = true;
			} else if (*src == '.' && is_float) {
				// 如果已经是小数点了，报错
				printf("在第%d行发生错误，原因：非法的数字（多个小数点）", current_line);
				exit(EXIT_FAILURE);
			}

			buffer[i++] = next();
		}

		buffer[i] = '\0';
		char *token_value = (char *)malloc((i + 1) * sizeof(char));
		strcpy(token_value, buffer);

		Token token = { is_float ? Token_Float : Token_Integer, token_value, current_line };
		return token;
	}
	// 生成字符串token（双引号包裹的字符串）
	if (*src == '\"') {
		next();
		int i = 0;

		while (*src != '\n' && *src != '\0') {
			if (*src == '\"' && buffer[i - 1] != '\\') {
				next();
				buffer[i] = '\0';
				char *token_value = (char *)malloc((i + 1) * sizeof(char));
				strcpy(token_value, buffer);

				Token token = { Token_String, token_value, current_line };
				return token;
			} else {
				buffer[i++] = next();
			}
		}

		printf("在第%d行发生错误，原因：字符串未闭合", current_line);
		exit(EXIT_FAILURE);
	}
	// 生成符号token (+ - * / = ; . , ! != == < > <= >= ( ) { } " )
	switch (*src) {
		char *token_value;
		Token token;
	case '+':
		return set_token(Token_Add);
	case '-':
		return set_token(Token_Subtract);
	case '*':
		return set_token(Token_Multiply);
	case '/':
		return set_token(Token_Divide);
	case '=':
		return set_complex_token(buffer, '=', Token_Equal, Token_Assign);
	case ';':
		return set_token(Token_Separator);
	case '.':
		return set_token(Token_Dot);
	case ',':
		return set_token(Token_Comma);
	case '!':
		return set_complex_token(buffer, '=', Token_NotEqual, Token_Not);
	case '<':
		return set_complex_token(buffer, '=', Token_LessThanEqual, Token_LessThan);
	case '>':
		return set_complex_token(buffer, '=', Token_GreaterThanEqual, Token_GreaterThan);
	case '(':
		return set_token(Token_LeftParen);
	case ')':
		return set_token(Token_RightParen);
	case '{':
		return set_token(Token_LeftBrace);
	case '}':
		return set_token(Token_RightBrace);
	}

	//结尾
	if (*src == '\0') {
		// 生成EOF token
		Token token = { Token_EOF, NULL, current_line };
		return token;
	}

	// 其他情况
	printf("在第%d行发生错误，原因：未知符号%c", current_line, *src);
	exit(EXIT_FAILURE);
}

bool is_keyword(char *str) {
	for (int i = 0; i < keywords_count; i++) {
		if (strcmp(str, keywords[i]) == 0) {
			return true;
		}
	}

	return false;
}

Token set_token(TokenType token_tpye) {
	char *token_value = (char *)malloc(2 * sizeof(char));
	token_value[0] = next();
	token_value[1] = '\0';
	Token token = { token_tpye, token_value, current_line };
	return token;
}

Token set_complex_token(char *buffer, char condition, TokenType true_token_type, TokenType false_token_type) {
	buffer[0] = next();
	if (*src == condition) {
		buffer[1] = next();
		buffer[2] = '\0';
		char *token_value = (char *)malloc(3 * sizeof(char));
		strcpy(token_value, buffer);
		Token token = { true_token_type, token_value, current_line };
		return token;
	} else {
		buffer[1] = '\0';
		char *token_value = (char *)malloc(2 * sizeof(char));
		strcpy(token_value, buffer);
		Token token = { false_token_type, token_value, current_line };
		return token;
	}
}

void print_token(Token token) {
	printf("Token:%-25sTokenType:", token.value);
	switch (token.type) {
		case Token_Keyword:
			printf("Keyword\n");
			break;
		case Token_Identifier:
			printf("Identifier\n");
			break;
		case Token_Integer:
			printf("Integer\n");
			break;
		case Token_Float:
			printf("Float\n");
			break;
		case Token_Bool:
			printf("Bool\n");
			break;
		case Token_String:
			printf("String\n");
			break;
		case Token_Positive:
			printf("Positive\n");
			break;
		case Token_Negative:
			printf("Negative\n");
			break;
		case Token_Add:
			printf("Add\n");
			break;
		case Token_Subtract:
			printf("Subtract\n");
			break;
		case Token_Multiply:
			printf("Multiply\n");
			break;
		case Token_Divide:
			printf("Divide\n");
			break;
		case Token_Assign:
			printf("Assign\n");
			break;
		case Token_Separator:
			printf("Separator\n");
			break;
		case Token_LeftParen:
			printf("LeftParen\n");
			break;
		case Token_RightParen:
			printf("RightParen\n");
			break;
		case Token_LeftBrace:
			printf("LeftBrace\n");
			break;
		case Token_RightBrace:
			printf("RightBrace\n");
			break;
		case Token_GreaterThan:
			printf("GreaterThan\n");
			break;
		case Token_LessThan:
			printf("LessThen\n");
			break;
		case Token_GreaterThanEqual:
			printf("GreaterThanEqual\n");
			break;
		case Token_LessThanEqual:
			printf("LessThenEqual\n");
			break;
		case Token_Equal:
			printf("Equal\n");
			break;
		case Token_NotEqual:
			printf("NotEqual\n");
			break;
		case Token_And:
			printf("And\n");
			break;
		case Token_Or:
			printf("Or\n");
			break;
		case Token_Not:
			printf("Not\n");
			break;
		case Token_Dot:
			printf("Dot\n");
			break;
		case Token_Comma:
			printf("Comma\n");
			break;
		case Token_Quote:
			printf("Quote\n");
			break;
		case Token_Unknown:
			printf("Unknown\n");
			break;
		case Token_EOF:
			printf("EOF\n");
			break;
	}
}
