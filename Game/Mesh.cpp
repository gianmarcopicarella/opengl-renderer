#include "Mesh.h"

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

bool Mesh::load_mesh(std::string file)
{
	/*
	Assimp::Importer file_importer;
	const aiScene * scene = file_importer.ReadFile(file, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
	
	if (!scene) {
		fprintf(stderr, "Assimp mesh loading error: %s", file_importer.GetErrorString());
		exit(EXIT_FAILURE);
	}

	*/


	return false;
}

void Mesh::render()
{
}

void Mesh::init_from_scene(const aiScene* scene)
{
	/*this->entries.resize(scene->mNumMeshes);

	for (int i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];
		this->init_mesh(i, mesh);
	}
	*/

}

void Mesh::init_mesh(int index, const aiMesh* mesh)
{
	/*
	this->entries[index].MaterialIndex = mesh->mMaterialIndex;
	std::vector<Vertex> vertices;
	std::vector<int> indices;

	const aiVector3D zero(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < mesh->mNumVertices; i++) {
		const aiVector3D pos = mesh->mVertices[i];
		const aiVector3D normal = mesh->HasNormals() ? mesh->mVertices[i] : zero;
		const aiVector3D texcoord = mesh->HasTextureCoords(0) ? mesh->mVertices[i] : zero;
	
		Vertex v(glm::vec3(pos.x, pos.y, pos.z), 
			glm::vec3(normal.x, normal.y, normal.z), 
			glm::vec3(texcoord.x, texcoord.y, texcoord.z));
		
		vertices.push_back(v);
	}

	for (int i = 0; i < mesh->mNumFaces; i++) {
		const aiFace face = mesh->mFaces[i];
		indices.push_back(face.mIndices[0]);
		indices.push_back(face.mIndices[1]);
		indices.push_back(face.mIndices[2]);
	}

	this->entries[index].init(vertices, indices);*/
}

void Mesh::init_materials(const aiScene* scene, std::string file)
{
	/*for (int i = 0; i < scene->mNumMaterials; i++) {
		const aiMaterial* material = scene->mMaterials[i];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString path;

			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {

			}
		}
	}*/
}
