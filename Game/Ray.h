#pragma once
#include "Window.h"
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

struct Ray {
	glm::vec3 direction;
	glm::vec3 origin;
};

struct RayInfo {
	glm::vec3 collision_point[2];
	short collision_num;
};

static Ray get_ray_from_mouse(Window * win, glm::mat4 projection, glm::mat4 view, glm::vec3 camera_position) {
	
	double x, y;
	win->get_cursor_position(&x, &y);

	// Normalized coord System (opengl)
	x = (2.f * x) / (float)win->get_width() - 1.0f;
	y = 1.0f - (2.f * y) / (float)win->get_height();

	//printf("%f, %f\n", x, y);

	glm::vec4 coord_4d = { x, y, -1.f, 1.0f };
	coord_4d = glm::inverse(projection) * coord_4d;
	coord_4d.z = -1.f;
	coord_4d.w = 0;

	// dal view space al world space
	coord_4d = glm::normalize(glm::inverse(view) * coord_4d);

	struct Ray ray;
	ray.direction = glm::vec3(coord_4d.x, coord_4d.y, coord_4d.z);
	ray.origin = camera_position;
	return ray;
}
