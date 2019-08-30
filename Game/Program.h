#pragma once
#include "Shader.h"
#include <vector>

class Program {
public:
	Program();
	~Program();
	void add_shader(Shader shader);
	void compile();
	void bind();
	GLuint get_uniform_location(const char * name);

private:
	GLuint id;
};