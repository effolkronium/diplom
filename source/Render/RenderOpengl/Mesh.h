#pragma once
#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <assimp/scene.h>

class Shader;

inline static glm::mat4 Assimp2Glm(const aiMatrix4x4& from)
{
	return glm::mat4(
		from.a1, from.b1, from.c1, from.d1,
		from.a2, from.b2, from.c2, from.d2,
		from.a3, from.b3, from.c3, from.d3,
		from.a4, from.b4, from.c4, from.d4
	);
}

struct Vertex {
	inline static constexpr size_t c_maxBonePerVertexCount = 8;

    glm::vec3 Position{};
    glm::vec3 Normal{};
    glm::vec2 TexCoords{};
	std::uint32_t BoneIDs[c_maxBonePerVertexCount] = { 0 };
	float Weights[c_maxBonePerVertexCount] = { 0 };
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
	std::string glslName;
};

class Mesh {
public:
    // mesh data
    std::vector<Vertex>       m_vertices;
    std::vector<unsigned int> m_indices;
    std::vector<Texture>      m_textures;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
	~Mesh();

    void Draw(Shader& shader);
private:
    //  render data
    unsigned int VAO, VBO, EBO;
};