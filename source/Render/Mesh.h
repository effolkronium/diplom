#pragma once
#include <string>
#include <vector>
#include <array>
#include <map>
#include <glm/glm.hpp>
#include <assimp/scene.h>

namespace RenderCommon
{
	struct Vertex {
        inline static constexpr size_t c_maxBonePerVertexCount = 8;

		glm::vec3 Position{};
		glm::vec3 Normal{};
		glm::vec2 TexCoords{};
        std::uint32_t BoneIDs[c_maxBonePerVertexCount] = { 0 };
        float Weights[c_maxBonePerVertexCount] = { 0 };
	};

    struct Texture {
        enum class Type {
            diffuse,
            specular
        } type;

        std::string path;
        std::string glslName;

        static std::string toString(Type type);
    };

    class Mesh {
    public:
        Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::vector<Texture> textures);
        ~Mesh();
    public:
        std::vector<Vertex>   m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<Texture>  m_textures;
    };
}