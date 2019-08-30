#include "Loader.h"

char* Loader::get_file_as_string(const char* file)
{
	FILE* fp = fopen(file, "r");
	if (!fp) {
		fprintf(stderr, "Error during Loader::get_file_as_string");
		exit(EXIT_FAILURE);
	}
	char* buffer = new char[4096];
	int read = fread(buffer, sizeof(char), 4096, fp);
	buffer[read] = '\0';
	fclose(fp);
	return buffer;
}
