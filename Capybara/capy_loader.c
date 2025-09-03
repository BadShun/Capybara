#include "capy_loader.h"
#include <stdio.h>
#include <stdlib.h>

char *read_file(char *file_name) {
	FILE *file = fopen(file_name, "r");

	if (!file) {
		printf("未找到 %s", file_name);
		exit(EXIT_FAILURE);
	}

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	rewind(file);

	char *content = (char *)malloc((file_size + 1) * sizeof(char));
	size_t size = fread(content, sizeof(char), file_size, file);
	content[size * sizeof(char)] = '\0';
	fclose(file);

	return content;
}
