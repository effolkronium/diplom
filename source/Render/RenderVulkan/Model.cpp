#include "Model.h"
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <stdexcept>
#include "stb_image.h"
#include <filesystem>

using namespace std::literals;
namespace fs = std::filesystem;

namespace VulkanRender
{
    Model::Model(std::filesystem::path path, std::vector<std::pair<std::filesystem::path, Texture::Type>> textures):
        m_textures{ std::move(textures) }
    {
        const aiScene* scene = m_import.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            throw std::runtime_error{ "ERROR::ASSIMP::"s + m_import.GetErrorString() };

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
}