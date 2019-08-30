#include "Program.h"

Program::Program()
{
	this->id = glCreateProgram();
}

Program::~Program()
{
	glDeleteProgram(id);
}

void Program::add_shader(Shader shader)
{
	glAttachShader(id, shader.get_id());
}

void Program::compile()
{
	glLinkProgram(id);
}

void Program::bind()
{
	glUseProgram(id);
}

GLuint Program::get_uniform_location(const char* name) {
	return glGetUniformLocation(id, name);
}
