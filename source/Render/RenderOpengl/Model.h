#pragma once

#include "Mesh.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <map>
#include <filesystem>

struct BoneInfo
{
	aiMatrix4x4 BoneOffset;
	aiMatrix4x4 FinalTransformation;
};

class Model
{
public:
	Model(std::filesystem::path path, std::vector<std::filesystem::path> textures);

    void Draw(Shader& shader);
	void BoneTransform(float TimeInSeconds, std::vector<aiMatrix4x4>& Transforms);
private:
	Assimp::Importer m_import;

	std::vector<std::filesystem::path> m_textures;

    std::vector<Mesh> meshes;
    std::vector<Texture> textures_loaded;

	std::map<std::string, std::uint32_t> m_BoneMapping; // maps a bone name to its index
	std::uint32_t m_NumBones = 0;
	std::vector<BoneInfo> m_BoneInfo;
	aiMatrix4x4 m_GlobalInverseTransform;

	unsigned int Model::TextureFromFile(std::filesystem::path path);

    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
        std::string typeName);

	void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform);

	void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);

	int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
	int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
};