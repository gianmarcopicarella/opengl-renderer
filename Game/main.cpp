#include "Window.h"
#include "Shader.h"
#include "Program.h"
#include "Loader.h"
#include "AnimationController.h"
#include <vector>
#include "Ray.h"
#include "Plane.h"
#include "BoundingBox.h"
#include <string>
#include <map>
#include <cassert>

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags


// Memory Leak detection library
#include <vld.h>

#define SHADER_PATH "./shaders/"
#define MODELS_PATH "./models/"
  

static double DT;
static glm::mat4 MVP;

static float cross_2d_product(glm::vec2 a, glm::vec2 b) {
	return a.x* b.y - a.y * b.x;
}

static bool is_in_triangle(glm::vec2 point, glm::vec2 a, glm::vec2 b, glm::vec2 c) {

	float f1 = cross_2d_product(b - a, point - a);
	float f2 = cross_2d_product(c - b, point - b);
	float f3 = cross_2d_product(a - c, point - c);

	printf("%f, %f, %f\n", f1, f2, f3);

	return f1 >= 0 && f2 >= 0 && f3 >= 0;
}

static glm::mat4 lookAt(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
	glm::vec3 front = normalize(target - position);
	glm::vec3 right = normalize(cross(up, front));

	glm::mat4 rot = glm::mat4(1);
	rot[0] = glm::vec4(right, 0);
	rot[1] = glm::vec4(cross(front, right), 0);
	rot[2] = glm::vec4(front, 0);

	return rot;
}

typedef struct GLTransform {
	glm::mat4 model;
	glm::mat4 translate;
	glm::mat4 rotate;
	glm::mat4 scale;

} GLTransform;

#define MAX_MESHES 10

typedef struct MeshEntry {
	int base_vertex;
	int base_index;
	int index_count;
} MeshEntry;

typedef struct GLElement {
	// element info
	char* name;
	GLTransform transform;

	// GL binding info
	GLuint VAO;
	GLuint VBO;
	unsigned int vertex_count;

	// in case of indexed rendering
	GLuint IBO;
	unsigned int indices_count;

	// Assimp loading multiple mesh
	MeshEntry meshes[MAX_MESHES];
	int mesh_count = 0;

	// shader info
	Program* shader_program;
	unsigned int attrib_count;
	GLuint mvp_location;

} GLElement;

typedef struct GLElementAABB {

	GLElement* element;
	AABoundingBox* aabb;

} GLElementAABB;

typedef struct Vertex {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 tcords;

	glm::ivec3 joints_indices;
	glm::vec3 joints_weight;

} Vertex;


void init_element_from_data(GLElement* el, Vertex vertices[], int vertices_count, bool has_animation) {
	glGenVertexArrays(1, &el->VAO);
	glBindVertexArray(el->VAO);

	glGenBuffers(1, &(el->VBO));
	glBindBuffer(GL_ARRAY_BUFFER, el->VBO);

	glBufferData(GL_ARRAY_BUFFER, vertices_count * sizeof(Vertex), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	if (has_animation) {
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(3, 3, GL_INT, sizeof(Vertex), (void*)(sizeof(glm::vec3) * 2 + sizeof(glm::vec2)));
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3) * 3 + sizeof(glm::vec2)));
		el->attrib_count += 2;
	}

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3) * 2));


	el->attrib_count += 3;
	el->vertex_count = vertices_count;
	el->transform = { glm::mat4(1), glm::mat4(1), glm::mat4(1), glm::scale(glm::mat4(1.f), glm::vec3(1,1,1)) };
}

void init_indexed_rendering_element(GLElement* el, unsigned int indices[], unsigned int indices_count) {
	glGenBuffers(1, &el->IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, el->IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	el->indices_count = indices_count;
}
GLElementAABB* init_aabb_element(char * name, Vertex* data, unsigned int vertices_count, bool has_norm, bool has_tcords) {
	GLElement* el = new GLElement();
	el->name = name;
	
	init_element_from_data(el, data, vertices_count, false);
	
	GLElementAABB* el_aabb = new GLElementAABB();
	el_aabb->element = el;
	
	glm::vec3 min(INT32_MAX, INT32_MAX, INT32_MAX);
	glm::vec3 max(INT32_MIN, INT32_MIN, INT32_MIN);

	for (int i = 0; i < vertices_count; i++) {
		
		if (min.x > data[i].pos.x) min.x = data[i].pos.x;
		if (min.y > data[i].pos.y) min.y = data[i].pos.y;
		if (min.z > data[i].pos.z) min.z = data[i].pos.z;

		if (max.x < data[i].pos.x) max.x = data[i].pos.x;
		if (max.y < data[i].pos.y) max.y = data[i].pos.y;
		if (max.z < data[i].pos.z) max.z = data[i].pos.z;

	}
	
	el_aabb->aabb = new AABoundingBox(min, max);
	el_aabb->aabb->refresh_min_max(el->transform.model[3]);

	return el_aabb;
}
void gl_render_indexed(GLElement* el) {
	glBindVertexArray(el->VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, el->IBO);
	el->shader_program->bind();
	for (int i = 0; i < el->attrib_count; glEnableVertexAttribArray(i++));
	glUniformMatrix4fv(el->mvp_location, 1, GL_FALSE, glm::value_ptr(MVP));
	glDrawElements(GL_TRIANGLES, el->indices_count, GL_UNSIGNED_INT, (void*)0);
	for (int i = 0; i < el->attrib_count; glDisableVertexAttribArray(i++));
	glBindVertexArray(0);
}
void lookAt_and_translate_element(GLElement* el, glm::vec3 to, float speed) {
	glm::vec3 dir = to - glm::vec3(el->transform.translate[3]);

	glm::vec3 dir_norm = glm::normalize(dir);

	el->transform.rotate = lookAt(glm::vec3(0, 0, 0), dir_norm, glm::vec3(0, 1, 0));

	// per evitare scatti della mesh quando il mouse punta sull'origine della mesh, applico la traslazione solo quando la distanza tra la mesh ed il punto colpiton supera un certo numero
	if (glm::length(dir) > 0.4f) {
		el->transform.translate = glm::translate(el->transform.translate, dir_norm * speed * (float)DT);
	}
		
	el->transform.model = el->transform.scale * el->transform.translate * el->transform.rotate;
}



typedef struct GLElementAnimated {
	GLElement* element;
	AnimationController * animator = nullptr;
} GLElementAnimated;

void free_animated_element(GLElementAnimated * el) {
	delete el->animator;
	delete el->element;
	delete el;
}
void render_animated_model(GLElementAnimated* el) {
	glBindVertexArray(el->element->VAO);
	el->element->shader_program->bind();
	for (int i = 0; i < el->element->attrib_count; glEnableVertexAttribArray(i++));

	el->animator->send_transform_to_shader(el->element->shader_program->get_uniform_location("bones_transform"));
	glUniformMatrix4fv(el->element->mvp_location, 1, GL_FALSE, glm::value_ptr(MVP));
	//glDrawElements(GL_TRIANGLES, el->element->indices_count, GL_UNSIGNED_INT, (void*)0);

	for (int i = 0; i < el->element->mesh_count; i++) {
		glDrawElementsBaseVertex(GL_TRIANGLES,
			el->element->meshes[i].index_count,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * el->element->meshes[i].base_index),
			el->element->meshes[i].base_vertex);
	}

	for (int i = 0; i < el->element->attrib_count; glDisableVertexAttribArray(i++));
	glBindVertexArray(0);
}

GLElementAnimated* init_element_from_animated_file(const char* filepath) {

	GLElementAnimated* el = new GLElementAnimated();
	el->element = new GLElement();
	el->element->name = (char*)"Assimp model";
	
	const aiScene* scene = aiImportFile(filepath,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType);

	if (!scene) {
		printf("Error during file opening\n");
		exit(EXIT_FAILURE);
	}

	int num_vertices = 0, num_indices = 0;

	/* initialization of meshes counting some data needed later */
	for (int i = 0; i < scene->mNumMeshes; i++) {
		el->element->meshes[i].index_count = scene->mMeshes[i]->mNumFaces * 3;
		el->element->meshes[i].base_vertex = num_vertices;
		el->element->meshes[i].base_index = num_indices;

		num_vertices += scene->mMeshes[i]->mNumVertices;
		num_indices += scene->mMeshes[i]->mNumFaces * 3;
	}

	Vertex* vertices = (Vertex*)calloc(num_vertices, sizeof(Vertex));
	unsigned int* indices = (unsigned int*)calloc(num_indices, sizeof(unsigned int));
	short* counter = (short*)calloc(num_vertices, sizeof(short));

	if (scene->HasAnimations()) {
		el->animator = new AnimationController(scene);
	}

	el->element->mesh_count = scene->mNumMeshes;

	for (int i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];

		/* add the vertices needed to render the mesh */
		for (int k = 0; k < mesh->mNumVertices; k++) {
			int index = k + el->element->meshes[i].base_vertex;
			vertices[index].pos = glm::vec3(mesh->mVertices[k].x, mesh->mVertices[k].y, mesh->mVertices[k].z);
			vertices[index].norm = glm::vec3(mesh->mNormals[k].x, mesh->mNormals[k].y, mesh->mNormals[k].z);
			vertices[index].tcords = mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][k].x, mesh->mTextureCoords[0][k].y) : glm::vec2(0, 0);
		}

		/* add the indices to render the current mesh */
		for (int k = 0; k < mesh->mNumFaces; k++) {
			int index = 3 * k + el->element->meshes[i].base_index;
			indices[index] = mesh->mFaces[k].mIndices[0];
			indices[index + 1] = mesh->mFaces[k].mIndices[1];
			indices[index + 2] = mesh->mFaces[k].mIndices[2];
		}

		/* if the current mesh has bones then add weights and index of the current bone to the vertices affected by it */
		for (int k = 0; k < mesh->mNumBones; k++) {
			aiBone* bone = mesh->mBones[k];
			std::string bone_name(bone->mName.data);
			SkeletalNode* my_bone = el->animator->get_node_by_name(bone_name);
			my_bone->offset_transform = mat4_cast(bone->mOffsetMatrix);

			for (int j = 0; j < bone->mNumWeights; j++) {
				aiVertexWeight weight = bone->mWeights[j];
				int vertex_id = el->element->meshes[i].base_vertex + weight.mVertexId;

				if (counter[vertex_id] < 3) {
					vertices[vertex_id].joints_weight[counter[vertex_id]] = weight.mWeight;
					vertices[vertex_id].joints_indices[counter[vertex_id]] = my_bone->index_transform;
					counter[vertex_id]++;
				}
			}
		}
	}

	/* if model has animations then normalize the weights per vertex assigned previously */
	if (scene->HasAnimations()) {
		for (int i = 0; i < scene->mNumMeshes; i++) {
			const aiMesh* mesh = scene->mMeshes[i];

			for (int k = 0; k < mesh->mNumVertices; k++) {
				float total = 1.f / vertices[k].joints_weight[0] + vertices[k].joints_weight[1] + vertices[k].joints_weight[2];
				vertices[k].joints_weight[0] *= total;
				vertices[k].joints_weight[1] *= total;
				vertices[k].joints_weight[2] *= total;
			}
		}
	}

	init_element_from_data(el->element, vertices, num_vertices, scene->HasAnimations());
	init_indexed_rendering_element(el->element, indices, num_indices);

	free(counter);
	free(vertices);
	free(indices);
	aiReleaseImport(scene);

	return el;
}

std::map<std::string, Program*> cache;
Loader loader;

void free_GLElementAABB(GLElementAABB* el) {
	delete el->element;
	delete el->aabb;
	delete el;
}

int assign_shader_to_element(GLElement* el, const char* name) {

	if (el == NULL && name == NULL) {
		for (auto& x : cache) {
			delete x.second;
		}
		cache.clear();
		return 0;
	}

	std::map<std::string, Program*>::iterator iter = cache.find(name);

	if (iter != cache.end()) {
		el->shader_program = iter->second;
		return 0;
	}

	char full_path[1024];
	strcpy(full_path, SHADER_PATH);
	strcat(full_path, name);
	strcat(full_path, ".vs");

	Shader vs(loader.get_file_as_string(full_path), GL_VERTEX_SHADER);
	
	int index = strlen(SHADER_PATH) + strlen(name);
	full_path[index] = 0;
	strcat(full_path, ".fs");

	Shader fs(loader.get_file_as_string(full_path), GL_FRAGMENT_SHADER);

	Program * pr = new Program();
	pr->add_shader(vs);
	pr->add_shader(fs);
	pr->compile();

	cache.insert(std::pair<std::string, Program*>(name, pr));
	el->shader_program = pr;
	return 0;
}

int main(int argc, char** argv) {

	Window* win = new Window(1280, 720, "Application");
	
	GLElementAnimated* model = init_element_from_animated_file("./models/model.dae");
	assign_shader_to_element(model->element, "player_anim");

	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	printf("%s\n", glGetString(GL_VERSION));

	model->element->transform.translate = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
	//model->element->transform.scale = glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
	model->element->mvp_location = model->element->shader_program->get_uniform_location("mvp");
	
	model->element->transform.model = model->element->transform.rotate *
		model->element->transform.scale *
		model->element->transform.translate;

	//cube->element->transform.model = glm::translate(glm::mat4(1.0f), glm::vec3(0,1,0));
	//cube->element->mvp_location = cube->element->shader_program->get_uniform_location("mvp");

	glm::vec3 camera_pos = glm::vec3(0, 20, 20);
	glm::mat4 view = glm::lookAt(camera_pos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 proj = glm::perspective<float>(glm::radians(45.f), 1280.f / 720.f, 0.1f, 100.f);
	
	Plane* plane = new Plane(glm::vec3(0, 1, 0), glm::vec3(0, 0, 0));

	double last_time = glfwGetTime();
	DT = 0;

	model->animator->set_animation(0);

	while (!win->is_closed()) {
		
		double curr_time = glfwGetTime();
		DT = curr_time - last_time;
		last_time = curr_time;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.f, 0.f, 0.0f, 1.f);

		/*
		struct Ray ray = get_ray_from_mouse(win, proj, view, camera_pos);
		struct RayInfo ray_info = plane->hit_by_ray(ray);
		
		if (ray_info.collision_num > 0) {

			lookAt_and_translate_element(model->element, ray_info.collision_point[0], 2.f);
		}
		*/

		// update MVP for each GLelement
		MVP = proj * view * model->element->transform.model;
		
		model->animator->update(DT);

		render_animated_model(model);

		win->update();
	}
	
	// free memory
	delete win;
	delete plane;
	assign_shader_to_element(NULL, NULL);


	free_animated_element(model);

	return EXIT_SUCCESS;
}