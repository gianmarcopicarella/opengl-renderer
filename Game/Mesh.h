#pragma once

#include <string>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <vector>
#include <glm/vec3.hpp>

class Mesh {
public:
	Mesh();
	~Mesh();

	bool load_mesh(std::string file);
	void render();

private:

	void init_from_scene(const aiScene* scene);
	void init_mesh(int index, const aiMesh* mesh);
	void init_materials(const aiScene* scene, std::string file);

	struct Vertex {
		Vertex(glm::vec3 p, glm::vec3 n, glm::vec3 t) {
			position = p;
			normal = n;
			textcoord = t;
		}
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 textcoord;
	};

	/*struct MeshEntry {
		MeshEntry();

		~MeshEntry();

		bool init(const std::vector<Mesh::Vertex>& Vertices,
			const std::vector<int>& Indices);

		int VB;
		int IB;
		unsigned int NumIndices;
		unsigned int MaterialIndex;
	};
	*/
	

	//std::vector<MeshEntry> entries;
	//std::vector<Texture> textures;

};