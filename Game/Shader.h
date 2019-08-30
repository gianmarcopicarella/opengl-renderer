#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>


class Shader {
public:
	Shader(char * code, GLuint type);
	~Shader();
	GLuint get_id();
	GLuint get_type();

private:
	GLuint id, type;
};