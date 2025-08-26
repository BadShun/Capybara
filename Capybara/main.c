#include <stdio.h>
#include <stdbool.h>
#include "capy_loader.h"
#include "capy_lexer.h"
#include "capy_parser.h"
#include "capy_stack.h"

int main() {
	parse_program("test.capy");

	return 0;
}