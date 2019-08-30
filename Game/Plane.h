#pragma once
#include <glm/vec3.hpp>
#include "Ray.h"

class Plane {
public:
	Plane(glm::vec3 normal, glm::vec3 point);
	RayInfo hit_by_ray(Ray r);
private:
	glm::vec3 normal;
	glm::vec3 point;
};