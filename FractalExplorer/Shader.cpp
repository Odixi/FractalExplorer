#include "pch.h"

#include "Shader.h"

constexpr unsigned int shaderTypeToGlType(ShaderType type) {
	switch (type)
	{
	case ShaderType::Vertex:
		return GL_VERTEX_SHADER;
	case ShaderType::Fragment:
		return GL_FRAGMENT_SHADER;
	default:
		return 0;
	}
}

Shader::~Shader()
{
	if (m_id) {
		glDeleteShader(m_id);
	}
}

std::optional<Shader> Shader::compileShader(std::string source, ShaderType type)
{
	// Tee shader objeckti vasta kun onnistunut...
	auto id = glCreateShader(shaderTypeToGlType(type));
	const char* str = source.c_str();
	glShaderSource(id, 1, &str, nullptr);
	glCompileShader(id);

	int r;
	glGetShaderiv(id, GL_COMPILE_STATUS, &r);
	if (r == GL_FALSE) {
		int lenght;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &lenght);
		std::vector<char> msg;
		msg.resize(lenght);
		glGetShaderInfoLog(id, lenght, &lenght, msg.data());
		std::cout << "Failed to compile shader: " << msg.data() << std::endl;
		return std::nullopt;
	}
	Shader shader;
	shader.m_source = std::move(source);
	shader.m_id = id;

	return std::make_optional(std::move(shader));
}

ShaderProgram::~ShaderProgram()
{
	glDeleteProgram(m_id);
}

std::optional<ShaderProgram> ShaderProgram::createAndLink(const std::vector<Shader*>& shaders)
{
	auto id = glCreateProgram();

	for (const auto& shader : shaders) {
		glAttachShader(id, shader->getId());
	}

	glLinkProgram(id);
	glValidateProgram(id);

	int r;
	glGetProgramiv(id, GL_VALIDATE_STATUS, &r);
	if (r == GL_FALSE) {
		int lenght;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &lenght);
		std::vector<char> msg;
		msg.resize(lenght);
		glGetProgramInfoLog(id, lenght, &lenght, msg.data());
		std::cout << "Program validation failed: " << msg.data() << std::endl;
		return std::nullopt;
	}
	ShaderProgram program;
	program.m_id = id;

	for (const auto& shader : shaders) {
		glDetachShader(program.m_id, shader->getId());
	}

	return program;
}

void ShaderProgram::bind()
{
	glUseProgram(m_id);
}
