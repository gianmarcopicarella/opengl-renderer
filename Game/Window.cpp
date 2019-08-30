#include "Window.h"

static void error_callback(int code, char* message)
{
	fprintf(stderr, "Error: %s\n", message);
}
static void keypress_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

Window::Window(int width, int height, std::string name)
{
	glfwSetErrorCallback((GLFWerrorfun)error_callback);

	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}
	
	this->width = width;
	this->height = height;

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	this->window = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);
	glfwSetTime(0.0f);

	if (!this->window) {
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(this->window);
	glfwSwapInterval(1);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		exit(EXIT_FAILURE);
	}

	int major, minor, revision;
	glfwGetVersion(&major, &minor, &revision);
	printf("GLFW Version %d.%d\n", major, minor);

	glfwSetWindowSizeLimits(this->window, 640, 480, 1280, 720);
	glfwSetKeyCallback(this->window, (GLFWkeyfun)keypress_callback);
	int s_width, s_height;
	this->get_framebuffer_size(&s_width, &s_height);
	glViewport(0, 0, s_width, s_height);

}

Window::~Window()
{
	glfwDestroyWindow(this->window);
	glfwTerminate();
}

bool Window::is_closed()
{
	return glfwWindowShouldClose(this->window);
}

void Window::update()
{
	glfwSwapBuffers(this->window);
	glfwPollEvents();
}

void Window::get_framebuffer_size(int* width, int* height)
{
	glfwGetFramebufferSize(this->window, width, height);
}

void Window::get_cursor_position(double* x, double* y)
{
	glfwGetCursorPos(this->window, x, y);
	if (*x > this->width) *x = this->width;
	else if (*x < 0) *x = 0;
	
	if (*y > this->height) *y = this->height;
	else if (*y < 0) *y = 0;
}

int Window::get_width()
{
	return this->width;
}

int Window::get_height()
{
	return this->height;
}





