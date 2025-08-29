#pragma once

#include <stdbool.h>

typedef enum {
	Token_Keyword,
	Token_Identifier,
	Token_Integer,
	Token_Float,
	Token_Bool,
	Token_String,
	Token_Positive,
	Token_Negative,
	Token_Add,
	Token_Subtract,
	Token_Multiply,
	Token_Divide,
	Token_Assign,
	Token_Separator,
	Token_LeftParen,
	Token_RightParen,
	Token_LeftBrace,
	Token_RightBrace,
	Token_GreaterThan,
	Token_LessThan,
	Token_GreaterThanEqual,
	Token_LessThanEqual,
	Token_Equal,
	Token_NotEqual,
	Token_And,
	Token_Or,
	Token_Not,
	Token_Dot,
	Token_Comma,
	Token_Quote,
	Token_Unknown,
	Token_EOF
} TokenType;

typedef struct {
	TokenType type;
	char *value;
	int line;
} Token;

void lexer_init(char *content);
Token next_token();
Token set_token(TokenType token_tpye);
Token set_complex_token(char *buffer, char condition, TokenType true_token_type, TokenType false_token_type);
bool is_keyword(char *str);
void print_token(Token token);

extern const char *keywords[];
extern const int keywords_count;