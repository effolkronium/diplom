#pragma once

#include "Mesh.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <map>
#include <filesystem>

namespace VulkanRender
{
    class Model
    {
    public:
        using Textures = std::vector<std::pair<std::filesystem::path, Texture::Type>>;
        Model(std::filesystem::path path, std::vector<std::pair<std::filesystem::path, Texture::Type>> textures);

        std::vector<Mesh> meshes;
    private:
        Assimp::Importer m_import;

        void processNode(aiNode* node, const aiScene* scene);
        Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    private:
        std::vector<std::pair<std::filesystem::path, Texture::Type>> m_textures;
    };
}