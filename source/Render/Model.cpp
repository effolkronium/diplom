#include "Model.h"
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <stdexcept>
#include "stb_image.h"
#include <filesystem>
#include "stb_image.h"

using namespace std::literals;
namespace fs = std::filesystem;

namespace VulkanRender
{
	namespace
	{
		void AddBoneData(Vertex& v, int BoneID, float Weight)
		{
			for (int i = 0; i < Vertex::c_maxBonePerVertexCount; i++) {
				if (v.Weights[i] == 0.0) {
					v.BoneIDs[i] = BoneID;
					v.Weights[i] = Weight;

					return;
				}
			}

			throw std::runtime_error{ "BONE EXEED !!!!" };
		}
	}

    Model::Model(std::filesystem::path path, std::vector<std::pair<std::filesystem::path, Texture::Type>> textures, int animationNumber):
        m_textures{ std::move(textures) }, m_animationNumber{ animationNumber }
    {
		static std::map<std::string, std::unique_ptr<Assimp::Importer>> imports;

		auto findIt = imports.find(path.string());

		const aiScene* scene = nullptr;
		if(findIt == imports.end())
		{
			auto importer = std::make_unique<Assimp::Importer>();

			scene = importer->ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs);

			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
				throw std::runtime_error{ "ERROR::ASSIMP::"s + importer->GetErrorString() };


			m_import = importer.get();
			imports.emplace(path.string(), std::move(importer));
		}
		else
		{
			scene = findIt->second->GetScene();
			m_import = findIt->second.get();
		}

		

        processNode(scene->mRootNode, scene);
    }

    void Model::processNode(aiNode* node, const aiScene* scene)
    {
        // process all the node's meshes (if any)
        auto name = node->mName;

        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            meshes.push_back(processMesh(mesh, scene));
        }
        // then do the same for each of its children
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Texture>  textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }

            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }

		for (std::uint32_t i = 0; i < mesh->mNumBones; i++) {
			std::uint32_t BoneIndex = 0;
			std::string BoneName = mesh->mBones[i]->mName.data;

			if (m_BoneMapping.find(BoneName) == m_BoneMapping.end()) {
				// Allocate an index for a new bone
				BoneIndex = m_NumBones;
				m_NumBones++;
				BoneInfo bi;
				m_BoneInfo.push_back(bi);
				m_BoneInfo[BoneIndex].BoneOffset = mesh->mBones[i]->mOffsetMatrix;
				m_BoneMapping[BoneName] = BoneIndex;
			}
			else {
				BoneIndex = m_BoneMapping[BoneName];
			}

			for (std::uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; j++) {
				std::uint32_t VertexID = mesh->mBones[i]->mWeights[j].mVertexId;
				AddBoneData(vertices[VertexID], BoneIndex, mesh->mBones[i]->mWeights[j].mWeight);
			}
		}

        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }


        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        for (auto& texture : m_textures)
        {
            Texture meshTexture;
            meshTexture.type = texture.second;
            meshTexture.path = texture.first.string();

            std::string number;
            if (meshTexture.type == Texture::Type::diffuse)
                number = std::to_string(diffuseNr++);
            else if (meshTexture.type == Texture::Type::specular)
                number = std::to_string(specularNr++); // transfer unsigned int to stream
            else
                throw std::runtime_error{ "Invalid Texture::Type" };

            meshTexture.glslName = "material." + Texture::toString(meshTexture.type) + number;
            textures.push_back(std::move(meshTexture));
        }

        return Mesh(std::move(vertices), std::move(indices), std::move(textures));
    }

	void Model::BoneTransform(float TimeInSeconds, std::vector<aiMatrix4x4>& Transforms)
	{
		aiMatrix4x4 Identity;

		float TicksPerSecond = (float)(m_import->GetScene()->mAnimations[m_animationNumber]->mTicksPerSecond != 0 ? m_import->GetScene()->mAnimations[m_animationNumber]->mTicksPerSecond : 25.0f);
		float TimeInTicks = TimeInSeconds * TicksPerSecond;
		float AnimationTime = fmod(TimeInTicks, (float)m_import->GetScene()->mAnimations[m_animationNumber]->mDuration);

		ReadNodeHeirarchy(AnimationTime, m_import->GetScene()->mRootNode, Identity);

		Transforms.resize(m_NumBones);

		for (int i = 0; i < m_NumBones; i++) {
			Transforms[i] = m_BoneInfo[i].FinalTransformation;
		}
	}

	unsigned char* Model::loadTexture(const std::string& path, int& width, int& height)
	{
		struct LoadedDataInfo
		{
			unsigned char* data = nullptr;
			int width{}, height{};
		};

		static std::map<std::string, LoadedDataInfo> loadedData;

		auto findIt = loadedData.find(path);
		if (findIt == loadedData.end())
		{
			int channels{};
			unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

			LoadedDataInfo info;
			
			info.data = data;
			info.height = height;
			info.width = width;

			loadedData.emplace(path, info);
			return data;
		}


		width = findIt->second.width;
		height = findIt->second.height;
		return findIt->second.data;
	}

	void Model::ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform)
	{
		std::string NodeName(pNode->mName.data);

		const aiAnimation* pAnimation = m_import->GetScene()->mAnimations[m_animationNumber];

		aiMatrix4x4 NodeTransformation(pNode->mTransformation);

		const aiNodeAnim* pNodeAnim = [&]() -> const aiNodeAnim* {
			for (int i = 0; i < pAnimation->mNumChannels; i++) {
				const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

				if (pNodeAnim->mNodeName.data == NodeName) {
					return pNodeAnim;
				}
			}

			return nullptr;
		}();


		if (pNodeAnim) {
			// Interpolate scaling and generate scaling transformation matrix
			aiVector3D Scaling;
			CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
			aiMatrix4x4 ScalingM;
			aiMatrix4x4::Scaling({ Scaling.x, Scaling.y, Scaling.z }, ScalingM);

			// Interpolate rotation and generate rotation transformation matrix
			aiQuaternion RotationQ;
			CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
			aiMatrix4x4 RotationM = aiMatrix4x4(RotationQ.GetMatrix());

			// Interpolate translation and generate translation transformation matrix
			aiVector3D Translation;
			CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
			aiMatrix4x4 TranslationM;
			aiMatrix4x4::Translation({ Translation.x, Translation.y, Translation.z }, TranslationM);

			// Combine the above transformations
			NodeTransformation = TranslationM * RotationM * ScalingM;
		}

		aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;

		if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
			int BoneIndex = m_BoneMapping[NodeName];
			m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
		}

		for (int i = 0; i < pNode->mNumChildren; i++) {
			ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
		}
	}

	void Model::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		if (pNodeAnim->mNumScalingKeys == 1) {
			Out = pNodeAnim->mScalingKeys[0].mValue;
			return;
		}

		int ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
		int NextScalingIndex = (ScalingIndex + 1);
		assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
		float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
		float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
		assert(Factor >= 0.0f && Factor <= 1.0f);
		const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
		const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
		aiVector3D Delta = End - Start;
		Out = Start + Factor * Delta;
	}

	void Model::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		// we need at least two values to interpolate...
		if (pNodeAnim->mNumRotationKeys == 1) {
			Out = pNodeAnim->mRotationKeys[0].mValue;
			return;
		}

		int RotationIndex = FindRotation(AnimationTime, pNodeAnim);
		int NextRotationIndex = (RotationIndex + 1);
		assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
		float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
		float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
		assert(Factor >= 0.0f && Factor <= 1.0f);
		const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
		const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
		aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
		Out = Out.Normalize();
	}

	void Model::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		if (pNodeAnim->mNumPositionKeys == 1) {
			Out = pNodeAnim->mPositionKeys[0].mValue;
			return;
		}

		int PositionIndex = FindPosition(AnimationTime, pNodeAnim);
		int NextPositionIndex = (PositionIndex + 1);
		assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
		float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
		float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
		assert(Factor >= 0.0f && Factor <= 1.0f);
		const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
		const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
		aiVector3D Delta = End - Start;
		Out = Start + Factor * Delta;
	}

	int Model::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		assert(pNodeAnim->mNumScalingKeys > 0);

		for (int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
			if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
				return i;
			}
		}

		assert(0);

		return 0;
	}

	int Model::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		assert(pNodeAnim->mNumRotationKeys > 0);

		for (int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
			if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
				return i;
			}
		}

		assert(0);

		return 0;
	}

	int Model::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		for (int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
			if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
				return i;
			}
		}

		assert(0);

		return 0;
	}

}