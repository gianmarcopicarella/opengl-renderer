#include "Shader.h"

Shader::Shader(char* code, GLuint type)
{
	this->type = type;
	this->id = glCreateShader(type);

	glShaderSource(id, 1, &code, nullptr);
	glCompileShader(id);
	GLint success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar log[512];
		glGetShaderInfoLog(id, sizeof(log) / sizeof(GLchar), nullptr, log);
		fprintf(stderr, "Shader logger:\n%s", log);
		exit(EXIT_FAILURE);
	}

	free(code);

}

Shader::~Shader()
{
	glDeleteShader(id);
}

GLuint Shader::get_id()
{
	return id;
}

GLuint Shader::get_type()
{
	return type;
}
