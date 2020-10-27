#pragma once

#include "Mesh.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <map>
#include <filesystem>

namespace VulkanRender
{
    inline static glm::mat4 Assimp2Glm(const aiMatrix4x4& from)
    {
        return glm::mat4(
            from.a1, from.b1, from.c1, from.d1,
            from.a2, from.b2, from.c2, from.d2,
            from.a3, from.b3, from.c3, from.d3,
            from.a4, from.b4, from.c4, from.d4
        );
    }

    struct BoneInfo
    {
        aiMatrix4x4 BoneOffset;
        aiMatrix4x4 FinalTransformation;
    };

    class Model
    {
    public:
        using Textures = std::vector<std::pair<std::filesystem::path, Texture::Type>>;
        Model(std::filesystem::path path, std::vector<std::pair<std::filesystem::path, Texture::Type>> textures, int animationNumber);
        
        void BoneTransform(float TimeInSeconds, std::vector<aiMatrix4x4>& Transforms);

        static unsigned char* loadTexture(const std::string& path, int& width, int& height);

        std::vector<Mesh> meshes;
    private:
        Assimp::Importer* m_import;

        std::map<std::string, std::uint32_t> m_BoneMapping; // maps a bone name to its index
        std::uint32_t m_NumBones = 0;
        std::vector<BoneInfo> m_BoneInfo;
        aiMatrix4x4 m_GlobalInverseTransform;

        void processNode(aiNode* node, const aiScene* scene);
        Mesh processMesh(aiMesh* mesh, const aiScene* scene);

        void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform);

        void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
        void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
        void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);

        int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
        int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
        int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
    private:
        std::vector<std::pair<std::filesystem::path, Texture::Type>> m_textures;
        int m_animationNumber = 0;
    };
}