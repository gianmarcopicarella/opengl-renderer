#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <glm/vec2.hpp>

class Window {
public:
	Window(int width, int height, std::string name);
	~Window();
	bool is_closed();
	void update();
	void get_framebuffer_size(int* width, int* height);
	void get_cursor_position(double* x, double* y);
	int get_width();
	int get_height();

private:
	GLFWwindow* window;
	int width, height;
};
	