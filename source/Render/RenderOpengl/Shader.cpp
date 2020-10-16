#include "Shader.h"
#include "glad/glad.h"
#include <cassert>
#include "glm/gtc/type_ptr.hpp"

using namespace std::literals;

Shader::Shader(std::string vertexSource, std::string fragmentSource)
{
	GLuint vertex = compileShader(vertexSource.c_str(), GL_VERTEX_SHADER);
	GLuint fragment = compileShader(fragmentSource.c_str(), GL_FRAGMENT_SHADER);

	m_programID = linkShader(vertex, fragment);

	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

Shader::~Shader()
{
	if (m_programID)
		glDeleteProgram(m_programID);

	m_programID = 0;
}

void Shader::use()
{
	assert(m_programID);
	glUseProgram(m_programID);
}

void Shader::setBool(const std::string& name, bool value) const
{
	assert(m_programID);
	glUniform1i(glGetUniformLocation(m_programID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) const
{
	assert(m_programID);
	glUniform1i(glGetUniformLocation(m_programID, name.c_str()), value);
}
void Shader::setFloat(const std::string& name, float value) const
{
	assert(m_programID);
	glUniform1f(glGetUniformLocation(m_programID, name.c_str()), value);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat)
{
	glUniformMatrix4fv(glGetUniformLocation(m_programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec)
{
	glUniform3fv(glGetUniformLocation(m_programID, name.c_str()), 1, glm::value_ptr(vec));
}

GLuint Shader::compileShader(const char* source, GLenum shaderType)
{
	GLuint shader = 0;

	shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if(!success)
		reportError(shader, "shader compilation failed");

	return shader;
}

GLuint Shader::linkShader(GLuint vertex, GLuint fragment)
{
	GLuint programID = glCreateProgram();
	glAttachShader(programID, vertex);
	glAttachShader(programID, fragment);
	glLinkProgram(programID);

	GLint success = 0;
	glGetProgramiv(programID, GL_LINK_STATUS, &success);

	if (!success)
		reportError(programID, "shader linking failed");

	return programID;
}

void Shader::reportError(GLuint Id, std::string message)
{
	GLchar infoLog[512] = { 0 };
	glGetShaderInfoLog(Id, sizeof(infoLog), NULL, infoLog);
	throw std::runtime_error{ "ERROR: " + message + ":" + infoLog };
}
