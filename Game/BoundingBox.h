#pragma once
#include "Ray.h"

class AABoundingBox {
public:
	AABoundingBox(glm::vec3 min, glm::vec3 max);
	RayInfo hit_by_ray(Ray r);
	void refresh_min_max(glm::vec3 mesh_world_position);

private:
	glm::vec3 min;
	glm::vec3 max;
};