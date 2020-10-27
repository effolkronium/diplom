#include "Mesh.h"

namespace RenderCommon
{
	Mesh::~Mesh()
	{

	}


	Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::vector<Texture> textures) :
		m_vertices{ std::move(vertices) }, m_indices{ std::move(indices) }, m_textures{ std::move(textures) }
	{

	}

	std::string Texture::toString(Type type)
	{
		if (type == Type::diffuse)
			return "texture_diffuse";
		else if (type == Type::specular)
			return "texture_specular";
	}
}