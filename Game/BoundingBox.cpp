#include "BoundingBox.h"

AABoundingBox::AABoundingBox(glm::vec3 min, glm::vec3 max)
{
	this->min = min;
	this->max = max;
}

RayInfo AABoundingBox::hit_by_ray(Ray r)
{
	double t_min = INT_MIN, t_max = INT_MAX;

	if (r.direction.x != 0) {

		double tx0 = (min.x - r.origin.x) / r.direction.x;
		double tx1 = (max.x - r.origin.x) / r.direction.x;

		t_min = glm::max(t_min, glm::min(tx0, tx1));
		t_max = glm::min(t_max, glm::max(tx0, tx1));
	}

	if (r.direction.y != 0) {

		double ty0 = (min.y - r.origin.y) / r.direction.y;
		double ty1 = (max.y - r.origin.y) / r.direction.y;

		t_min = glm::max(t_min, glm::min(ty0, ty1));
		t_max = glm::min(t_max, glm::max(ty0, ty1));
	}

	if (r.direction.z != 0) {
		double tz0 = (min.z - r.origin.z) / r.direction.z;
		double tz1 = (max.z - r.origin.z) / r.direction.z;

		t_min = glm::max(t_min, glm::min(tz0, tz1));
		t_max = glm::min(t_max, glm::max(tz0, tz1));
	}

	RayInfo points;
	points.collision_num = 0;

	if (t_max >= t_min) {
		printf("coll: %f, %f\n", t_min, t_max);
		points.collision_num = 2;
		points.collision_point[0] = glm::vec3(t_min * r.direction.x + r.origin.x, t_min * r.direction.y + r.origin.y, t_min * r.direction.z + r.origin.z);
		points.collision_point[1] = glm::vec3(t_max * r.direction.x + r.origin.x, t_max * r.direction.y + r.origin.y, t_max * r.direction.z + r.origin.z);
	}
	else {
		printf("no coll\n");
	}
	return points;
}

void AABoundingBox::refresh_min_max(glm::vec3 mesh_world_position)
{
	this->min = this->min + mesh_world_position;
	this->max = this->max + mesh_world_position;
}
