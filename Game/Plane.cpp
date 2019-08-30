#include "Plane.h"

Plane::Plane(glm::vec3 normal, glm::vec3 point)
{
	this->normal = normal;
	this->point = point;
}

RayInfo Plane::hit_by_ray(Ray r)
{
	// creo un ogetto RayInfo che contiene tutte le informazione circa le collisioni
	struct RayInfo ray_info = {};

	// vettore che punta dal punto del piano all'origine del ray
	double dtp = glm::dot(r.origin - this->point, this->normal);
	
	// distanza tra l'origine del ray ed il suo punto proiettato sul piano 
	double th = glm::dot(r.direction, -this->normal);

	// se la distanza è quasi 0, allora l'origine del ray si trova sul piano e quindi il punto d'intersezione è:
	if (glm::abs(dtp) <= 0.00001f) {
		ray_info.collision_num = 1;
		ray_info.collision_point[0] = r.origin - this->point;
		printf("HIT POINTS: %f, %f, %f\n", ray_info.collision_point[0].x, ray_info.collision_point[0].y, ray_info.collision_point[0].z);
		return ray_info;
	}
	// se il valore ottenuto dal dot product è 0, allora la direzione del ray è parallela al piano e quindi non esiste un punto d'intersezione
	if (th == 0) {
		return ray_info;
	}

	// calcolo il punto di intersezione con il piano
	double rat = dtp * (1.0f / th);

	ray_info.collision_num = 1;
	ray_info.collision_point[0] = r.origin + glm::vec3(r.direction.x * rat, r.direction.y * rat, r.direction.z * rat);

	printf("HIT POINTS: %f, %f, %f\n", ray_info.collision_point[0].x, ray_info.collision_point[0].y, ray_info.collision_point[0].z);
	return ray_info;
}
