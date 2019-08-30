#include "Window.h"
#include "Shader.h"
#include "Program.h"
#include "Loader.h"
#include <glm/vec3.hpp>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Ray.h"
#include "Plane.h"
#include "BoundingBox.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string.h>
#include <string>
#include <map>
#include <set>

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
} Vertex;

typedef struct AnimatedVertex {
	Vertex vertex;

	glm::ivec3 joints_indices;
	glm::vec3 joints_weight;

} AnimatedVertex;



void init_indexed_rendering_element(GLElement* el, unsigned int indices[], unsigned int indices_count) {
	glGenBuffers(1, &el->IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, el->IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	el->indices_count = indices_count;
}
void init_element_from_data(GLElement* el, Vertex * data, unsigned int vertices_count, bool has_norm, bool has_tcords) {

	glGenVertexArrays(1, &el->VAO);
	glBindVertexArray(el->VAO);

	glGenBuffers(1, &(el->VBO));
	glBindBuffer(GL_ARRAY_BUFFER, el->VBO);

	if (has_norm && has_tcords) {
		glBufferData(GL_ARRAY_BUFFER, vertices_count * sizeof(Vertex), data, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3) * 2));

		el->attrib_count += 3;

		printf("HAS_NORM AND HAS_TCORDS\n");
	}
	else if (has_norm) {
		glBufferData(GL_ARRAY_BUFFER, vertices_count * sizeof(Vertex), data, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));

		el->attrib_count += 2;

		printf("HAS_NORM\n");
	}
	else if (has_tcords) {
		glBufferData(GL_ARRAY_BUFFER, vertices_count * sizeof(Vertex), data, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3) * 2));

		el->attrib_count += 2;

		printf("HAS_TCORDS\n");
	}
	else {
		glBufferData(GL_ARRAY_BUFFER, vertices_count * sizeof(Vertex), data, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

		el->attrib_count++;
	}

	el->vertex_count = vertices_count;
	el->transform = { glm::mat4(1), glm::mat4(1), glm::mat4(1), glm::scale(glm::mat4(1.f), glm::vec3(1,1,1)) };
}
GLElementAABB* init_aabb_element(char * name, Vertex* data, unsigned int vertices_count, bool has_norm, bool has_tcords) {
	GLElement* el = new GLElement();
	el->name = name;
	
	init_element_from_data(el, data, vertices_count, has_norm, has_tcords);
	
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
void gl_render_no_indexed(GLElement * el) {
	glBindVertexArray(el->VAO);
	el->shader_program->bind();
	for (int i = 0; i < el->attrib_count; glEnableVertexAttribArray(i++));
	glUniformMatrix4fv(el->mvp_location, 1, GL_FALSE, glm::value_ptr(MVP));
	glDrawArrays(GL_TRIANGLES, 0, el->vertex_count);
	for (int i = 0; i < el->attrib_count; glDisableVertexAttribArray(i++));
	glBindVertexArray(0);
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


static inline glm::vec3 vec3_cast(const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); }
static inline glm::vec2 vec2_cast(const aiVector3D& v) { return glm::vec2(v.x, v.y); }
static inline glm::quat quat_cast(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }
static inline glm::mat4 mat4_cast(const aiMatrix4x4& m) { return glm::transpose(glm::make_mat4(&m.a1)); }
static inline glm::mat4 mat4_cast(const aiMatrix3x3& m) { return glm::transpose(glm::make_mat3(&m.a1)); }




void init_animated_element_from_data(GLElement* el, AnimatedVertex vertices[], int vertices_count) {
	glGenVertexArrays(1, &el->VAO);
	glBindVertexArray(el->VAO);

	glGenBuffers(1, &(el->VBO));
	glBindBuffer(GL_ARRAY_BUFFER, el->VBO);

	glBufferData(GL_ARRAY_BUFFER, vertices_count * sizeof(AnimatedVertex), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (void*)(sizeof(glm::vec3)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (void*)(sizeof(glm::vec3) * 2));
	glVertexAttribIPointer(3, 3, GL_INT, sizeof(AnimatedVertex), (void*)(sizeof(Vertex)));
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (void*)(sizeof(Vertex) + sizeof(glm::vec3)));

	el->attrib_count = 5;
	el->vertex_count = vertices_count;
	el->transform = { glm::mat4(1), glm::mat4(1), glm::mat4(1), glm::scale(glm::mat4(1.f), glm::vec3(1,1,1)) };
}

GLElement* init_element_from_file(const char* filepath) {

	GLElement* el = new GLElement();
	el->name = (char*)"Assimp";

	const aiScene* scene = aiImportFile(filepath,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType);

	if (!scene) {
		printf("Error during file opening\n");
		exit(EXIT_FAILURE);
	}

	if (scene->mNumMeshes != 1) {
		printf("None or too much Meshes found\n");
		exit(EXIT_FAILURE);
	}

	const aiMesh* mesh = scene->mMeshes[0];

	// first vector for position, second vector for normals and third one for textureCoords
	Vertex* vertices = (Vertex*)calloc(mesh->mNumVertices, sizeof(Vertex));


	glm::mat4 blender_rotate_model = glm::rotate(glm::radians(-90.f), glm::vec3(1, 0, 0));

	for (int i = 0; i < mesh->mNumVertices; i++) {
		glm::mat4 res = blender_rotate_model * glm::translate(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
		vertices[i].pos = glm::vec3(res[3][0], res[3][1], res[3][2]);
		vertices[i].norm = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

		if (mesh->HasTextureCoords(0)) {
			vertices[i].tcords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
		}
		else {
			vertices[i].tcords = glm::vec2(0, 0);
		}
	}

	
	unsigned int* indices = (unsigned int*)calloc(mesh->mNumFaces*3, sizeof(unsigned int));

	for (int i = 0; i < mesh->mNumFaces; i++) {
		assert(mesh->mFaces[i].mNumIndices == 3);
		indices[3 * i] = mesh->mFaces[i].mIndices[0];
		indices[3 * i + 1] = mesh->mFaces[i].mIndices[1];
		indices[3 * i + 2] = mesh->mFaces[i].mIndices[2];
	}

	init_element_from_data(el, vertices, mesh->mNumVertices, mesh->HasNormals(), mesh->HasTextureCoords(0));
	init_indexed_rendering_element(el, indices, mesh->mNumFaces * 3);

	free(indices);
	free(vertices);

	aiReleaseImport(scene);

	return el;
}

typedef struct PositionKeyFrame {
	float time;
	glm::vec3 value;
} PositionKeyFrame;

typedef struct RotationKeyFrame {
	float time;
	glm::quat value;
} RotationKeyFrame;

typedef struct ScalingKeyFrame {
	float time;
	glm::vec3 value;
} ScalingKeyFrame;

typedef struct Animation {
	/* keyframes */

	PositionKeyFrame* position_keyframe;
	RotationKeyFrame* rotation_keyframe;
	ScalingKeyFrame* scaling_keyframe;

	int position_keycount;
	int rotation_keycount;
	int scaling_keycount;

	/* end keyframes */
} Animation;

#define MAX_ANIMATIONS 4

typedef struct SkeletalNode {
	std::string name;
	glm::mat4 final_transform;
	glm::mat4 m_transform;
	glm::mat4 offset_transform = glm::mat4(1.0f);
	int index_transform = -1;

	Animation animations[MAX_ANIMATIONS];

	SkeletalNode** children;
	int children_count;

} SkeletalNode;

typedef struct Animator {
	SkeletalNode* root;
	std::map<std::string, SkeletalNode*> bones;
	int bones_count;
	glm::mat4 inverse_bind_transform;
	glm::mat4* final_transforms;
	float length[MAX_ANIMATIONS];
	float ticks_per_second[MAX_ANIMATIONS];
	int current_animation = -1;
} Animator;

typedef struct GLElementAnimated {
	GLElement* element;
	Animator animator;
} GLElementAnimated;

void free_skeleton(SkeletalNode* node, int animation_count) {
	for (int i = 0; i < animation_count; i++) {
		free(node->animations[i].position_keyframe);
		free(node->animations[i].rotation_keyframe);
		free(node->animations[i].scaling_keyframe);
	}

	for (int i = 0; i < node->children_count; i++) {
		free_skeleton(node->children[i], animation_count);
	}

	free(node->children);
	delete node;
}
void free_animated_element(GLElementAnimated * el) {
	free_skeleton(el->animator.root, 1);
	delete el->animator.final_transforms;
	el->animator.bones.clear();
	delete el->element;
	delete el;
}
void render_animated_model(GLElementAnimated* el) {
	glBindVertexArray(el->element->VAO);
	el->element->shader_program->bind();
	for (int i = 0; i < el->element->attrib_count; glEnableVertexAttribArray(i++));

	glUniformMatrix4fv(el->element->shader_program->get_uniform_location("bones_transform"),
		el->animator.bones_count, GL_FALSE, glm::value_ptr(el->animator.final_transforms[0]));
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

SkeletalNode* parse_skeleton(const aiNode* node, Animator * animator, std::set<std::string> * bones, int* index) {
	SkeletalNode* my_node = new SkeletalNode();
	my_node->name = std::string(node->mName.data);
	my_node->m_transform = mat4_cast(node->mTransformation);

	if (bones->find(my_node->name) != bones->end()) {
		my_node->index_transform = *index;
		*index += 1;
		animator->bones.insert(std::pair<std::string, SkeletalNode*>(my_node->name, my_node));
	}

	my_node->children_count = node->mNumChildren;
	my_node->children = (SkeletalNode * *)calloc(node->mNumChildren, sizeof(SkeletalNode*));

	for (int i = 0; i < node->mNumChildren; i++) {
		my_node->children[i] = parse_skeleton(node->mChildren[i], animator, bones, index);
	}

	return my_node;
}
void parse_animation_recursive(SkeletalNode* node, Animator * animator, std::map<std::string, aiNodeAnim*>* aiNode_lookup) {
	int animation_index = animator->current_animation;
	
	if (aiNode_lookup->find(node->name) != aiNode_lookup->end()) {
		aiNodeAnim* node_anim_data = aiNode_lookup->find(node->name)->second;

		node->animations[animation_index].position_keycount = node_anim_data->mNumPositionKeys;
		node->animations[animation_index].rotation_keycount = node_anim_data->mNumRotationKeys;
		node->animations[animation_index].scaling_keycount = node_anim_data->mNumScalingKeys;

		node->animations[animation_index].position_keyframe = (PositionKeyFrame*)calloc(node_anim_data->mNumPositionKeys, sizeof(PositionKeyFrame));

		for (int i = 0; i < node_anim_data->mNumPositionKeys; i++) {
			node->animations[animation_index].position_keyframe[i].time = node_anim_data->mPositionKeys[i].mTime;
			node->animations[animation_index].position_keyframe[i].value = vec3_cast(node_anim_data->mPositionKeys[i].mValue);
		}

		node->animations[animation_index].rotation_keyframe = (RotationKeyFrame*)calloc(node_anim_data->mNumRotationKeys, sizeof(RotationKeyFrame));

		for (int i = 0; i < node_anim_data->mNumRotationKeys; i++) {
			node->animations[animation_index].rotation_keyframe[i].time = node_anim_data->mRotationKeys[i].mTime;
			node->animations[animation_index].rotation_keyframe[i].value = quat_cast(node_anim_data->mRotationKeys[i].mValue);
		}

		node->animations[animation_index].scaling_keyframe = (ScalingKeyFrame*)calloc(node_anim_data->mNumScalingKeys, sizeof(ScalingKeyFrame));

		for (int i = 0; i < node_anim_data->mNumScalingKeys; i++) {
			node->animations[animation_index].scaling_keyframe[i].time = node_anim_data->mScalingKeys[i].mTime;
			node->animations[animation_index].scaling_keyframe[i].value = vec3_cast(node_anim_data->mScalingKeys[i].mValue);
		}


		Animation temp = node->animations[animation_index];

		animator->length[animation_index] = glm::max(animator->length[animation_index],
			glm::max(temp.position_keyframe[temp.position_keycount - 1].time,
				glm::max(temp.rotation_keyframe[temp.rotation_keycount - 1].time,
					temp.scaling_keyframe[temp.scaling_keycount - 1].time)));
	}
	for (int i = 0; i < node->children_count; i++) {
		parse_animation_recursive(node->children[i], animator, aiNode_lookup);
	}
}
void parse_animation(const aiScene* scene, Animator * animator, int animation_index) {
	std::map<std::string, aiNodeAnim*> aiNode_lookup = {};
	for (unsigned int i = 0; i < scene->mAnimations[animation_index]->mNumChannels; i++) {
		aiNodeAnim* pNodeAnim = scene->mAnimations[animation_index]->mChannels[i];
		aiNode_lookup.insert(std::pair<std::string, aiNodeAnim*>(std::string(pNodeAnim->mNodeName.data), pNodeAnim));
	}
	animator->ticks_per_second[animation_index] = scene->mAnimations[animation_index]->mTicksPerSecond != 0 ?
		scene->mAnimations[animation_index]->mTicksPerSecond : 25.0f;
	parse_animation_recursive(animator->root, animator, &aiNode_lookup);
}

std::set<std::string> get_bones_set(const aiScene* scene) {
	std::set<std::string> bones = {};
	for (unsigned int k = 0; k < scene->mNumAnimations; k++) {
		for (unsigned int i = 0; i < scene->mAnimations[k]->mNumChannels; i++) {
			aiNodeAnim* pNodeAnim = scene->mAnimations[k]->mChannels[i];
			std::string bone_name(pNodeAnim->mNodeName.data);
			if (bones.find(bone_name) == bones.end()) {
				bones.insert(bone_name);
			}
		}
	}
	return bones;
}

GLElementAnimated* init_element_from_animated_file(const char* filepath) {

	GLElementAnimated* el = new GLElementAnimated();
	el->element = new GLElement();
	el->element->name = (char*)"Assimp";

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

	AnimatedVertex* vertices = (AnimatedVertex*)calloc(num_vertices, sizeof(AnimatedVertex));
	unsigned int* indices = (unsigned int*)calloc(num_indices, sizeof(unsigned int));
	short* counter = (short*)calloc(num_vertices, sizeof(short));

	if (scene->HasAnimations()) {
		el->animator.inverse_bind_transform = glm::inverse(mat4_cast(scene->mRootNode->mTransformation));
		int bone_counter = 0;
		std::set<std::string> bones = get_bones_set(scene);
		el->animator.root = parse_skeleton(scene->mRootNode, &el->animator, &bones, &bone_counter);
		el->animator.bones_count = bone_counter;
		el->animator.final_transforms = (glm::mat4*)calloc(bone_counter, sizeof(glm::mat4));
		el->animator.current_animation = 0;

		for (int i = 0; i < scene->mNumAnimations; i++) {
			parse_animation(scene, &el->animator, i);
		}
	}

	el->element->mesh_count = scene->mNumMeshes;

	for (int i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];

		/* add the vertices needed to render the mesh */
		for (int k = 0; k < mesh->mNumVertices; k++) {
			int index = k + el->element->meshes[i].base_vertex;
			vertices[index].vertex.pos = glm::vec3(mesh->mVertices[k].x, mesh->mVertices[k].y, mesh->mVertices[k].z);
			vertices[index].vertex.norm = glm::vec3(mesh->mNormals[k].x, mesh->mNormals[k].y, mesh->mNormals[k].z);
			vertices[index].vertex.tcords = mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][k].x, mesh->mTextureCoords[0][k].y) : glm::vec2(0, 0);
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
			SkeletalNode* my_bone = el->animator.bones.find(bone_name)->second;
			my_bone->offset_transform = mat4_cast(bone->mOffsetMatrix);

			for (int j = 0; j < bone->mNumWeights; j++) {
				aiVertexWeight weight = bone->mWeights[j];
				int vertex_id = el->element->meshes[i].base_vertex + weight.mVertexId;

				if (counter[vertex_id] < 3) {
					vertices[vertex_id].joints_weight[counter[weight.mVertexId]] = weight.mWeight;
					vertices[vertex_id].joints_indices[counter[weight.mVertexId]] = my_bone->index_transform;
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
	
	init_animated_element_from_data(el->element, vertices, num_vertices);
	init_indexed_rendering_element(el->element, indices, num_indices);

	free(counter);
	free(vertices);
	free(indices);
	aiReleaseImport(scene);

	return el;
}



unsigned int FindPosition(float AnimationTime, const SkeletalNode* node, int animation_index)
{
	Animation temp = node->animations[animation_index];

	for (unsigned int i = 0; i < temp.position_keycount - 1; i++) {
		if (AnimationTime < (float)temp.position_keyframe[i + 1].time) {
			return i;
		}
	}

	assert(0);

	return 0;
}
unsigned int FindRotation(float AnimationTime, const SkeletalNode* node, int animation_index)
{
	Animation temp = node->animations[animation_index];

	assert(temp.rotation_keycount > 0);

	for (unsigned int i = 0; i < temp.rotation_keycount - 1; i++) {
		if (AnimationTime < (float)temp.rotation_keyframe[i + 1].time) {
			return i;
		}
	}

	assert(0);

	return 0;
}
unsigned int FindScaling(float AnimationTime, const SkeletalNode* node, int animation_index)
{
	Animation temp = node->animations[animation_index];

	assert(temp.scaling_keycount > 0);

	for (unsigned int i = 0; i < temp.scaling_keycount - 1; i++) {
		if (AnimationTime < (float)temp.scaling_keyframe[i + 1].time) {
			return i;
		}
	}

	assert(0);

	return 0;
}
glm::vec3 CalcInterpolatedPosition(float AnimationTime, const SkeletalNode* node, int animation_index)
{
	Animation temp = node->animations[animation_index];

	if (temp.position_keycount == 1) {
		return temp.position_keyframe[0].value;
	}

	unsigned int ind = FindPosition(AnimationTime, node, animation_index);
	unsigned int next_ind = ind + 1;
	assert(next_ind < temp.position_keycount);
	float DeltaTime = (float)(temp.position_keyframe[next_ind].time - temp.position_keyframe[ind].time);
	float Factor = (AnimationTime - (float)temp.position_keyframe[ind].time) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	return glm::mix(temp.position_keyframe[ind].value, temp.position_keyframe[next_ind].value, Factor);
}
glm::quat CalcInterpolatedRotation(float AnimationTime, const SkeletalNode* node, int animation_index)
{
	Animation temp = node->animations[animation_index];

	// we need at least two values to interpolate...
	if (temp.rotation_keycount == 1) {
		return temp.rotation_keyframe[0].value;
	}

	unsigned int ind = FindRotation(AnimationTime, node, animation_index);
	unsigned int next_ind = ind + 1;
	assert(next_ind < temp.rotation_keycount);
	float DeltaTime = (float)(temp.rotation_keyframe[next_ind].time - temp.rotation_keyframe[ind].time);
	float Factor = (AnimationTime - (float)temp.rotation_keyframe[ind].time) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	return glm::slerp(temp.rotation_keyframe[ind].value, temp.rotation_keyframe[next_ind].value, Factor);
}
glm::vec3 CalcInterpolatedScaling(float AnimationTime, const SkeletalNode* node, int animation_index)
{
	Animation temp = node->animations[animation_index];

	if (temp.scaling_keycount == 1) {
		return temp.scaling_keyframe[0].value;
	}

	unsigned int ind = FindScaling(AnimationTime, node, animation_index);
	unsigned int next_ind = (ind + 1);
	assert(next_ind < temp.scaling_keycount);
	float DeltaTime = (float)(temp.scaling_keyframe[next_ind].time - temp.scaling_keyframe[ind].time);
	float Factor = (AnimationTime - (float)temp.scaling_keyframe[ind].time) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	return glm::mix(temp.scaling_keyframe[ind].value, temp.scaling_keyframe[next_ind].value, Factor);
}

void read_node_heirarchy(float animation_time, SkeletalNode * node, glm::mat4 * parent_transform, glm::mat4 * global_inverse_transform, int * animation_index) {
	glm::mat4 node_transformation = node->m_transform;

	if(node->index_transform != -1){
		
		// Interpolate scaling and generate scaling transformation matrix
		glm::mat4 scaling_m = glm::scale(glm::mat4(1.0f), CalcInterpolatedScaling(animation_time, node, *animation_index));

		// Interpolate rotation and generate rotation transformation matrix
		glm::mat4 rotation_m = glm::toMat4(CalcInterpolatedRotation(animation_time, node, *animation_index));

		// Interpolate translation and generate translation transformation matrix
		glm::mat4 translation_m = glm::translate(glm::mat4(1.0f), CalcInterpolatedPosition(animation_time, node, *animation_index));

		// Combine the above transformations
		node_transformation = translation_m * scaling_m * rotation_m;
	}

	// Combine with node Transformation with Parent Transformation
	glm::mat4 global_transformation = *parent_transform * node_transformation;

	if (node->index_transform != -1) {
		node->final_transform = *global_inverse_transform * global_transformation * node->offset_transform;
	}

	for (unsigned int i = 0; i < node->children_count; i++) {
		read_node_heirarchy(animation_time, node->children[i], &global_transformation, global_inverse_transform, animation_index);
	}
}
void bone_transform(float time_in_seconds, Animator* animator) {
	glm::mat4 root_parent = glm::rotate(glm::radians(-90.f), glm::vec3(1.0f, 0, 0));

	int animation_index = animator->current_animation;

	float time_in_ticks = time_in_seconds * animator->ticks_per_second[animation_index];
	float animation_time = fmod(time_in_ticks, animator->length[animation_index]);

	read_node_heirarchy(animation_time, animator->root, &root_parent, &animator->inverse_bind_transform, &animator->current_animation);

	for (auto& x : animator->bones) {
		int index = x.second->index_transform;
		animator->final_transforms[index] = x.second->final_transform;
		//printf("%f\t%f\t%f1\n", anim->final_transforms[index][3][0], anim->final_transforms[index][3][1], anim->final_transforms[index][3][2]);
	}
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
	//model->element->transform.scale = glm::scale(glm::vec3(0.7f, 0.7f, 0.7f));
	model->element->mvp_location = model->element->shader_program->get_uniform_location("mvp");
	
	model->element->transform.model = model->element->transform.rotate *
		model->element->transform.scale *
		model->element->transform.translate;

	//cube->element->transform.model = glm::translate(glm::mat4(1.0f), glm::vec3(0,1,0));
	//cube->element->mvp_location = cube->element->shader_program->get_uniform_location("mvp");

	glm::vec3 camera_pos = glm::vec3(0, 14, 18);
	glm::mat4 view = glm::lookAt(camera_pos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 proj = glm::perspective<float>(glm::radians(45.f), 1280.f / 720.f, 0.1f, 100.f);
	
	Plane* plane = new Plane(glm::vec3(0, 1, 0), glm::vec3(0, 0, 0));

	double last_time = glfwGetTime();
	DT = 0;

	while (!win->is_closed()) {
		
		double curr_time = glfwGetTime();
		DT = curr_time - last_time;
		last_time = curr_time;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.f, 0.f, 0.0f, 1.f);
		
		
		struct Ray ray = get_ray_from_mouse(win, proj, view, camera_pos);
		struct RayInfo ray_info = plane->hit_by_ray(ray);
		
		if (ray_info.collision_num > 0) {

			lookAt_and_translate_element(model->element, ray_info.collision_point[0], 2.f);
		}
		

		// update MVP for each GLelement
		MVP = proj * view * model->element->transform.model;

		bone_transform(glfwGetTime(), &model->animator);
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