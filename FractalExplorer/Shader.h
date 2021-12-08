#pragma once

enum class ShaderType {
	Vertex,
	Fragment
};

class Shader
{
public:
	Shader(const Shader& ) = delete;
	Shader(Shader&& other) noexcept{
		this->m_source = std::move(other.m_source);
		this->m_id = other.m_id;
		other.m_id = 0;
	}
	Shader& operator=(const Shader&) = delete;
	Shader& operator=(Shader&& other) noexcept{
		this->m_source = std::move(other.m_source);
		this->m_id = other.m_id;
		other.m_id = 0;
		return *this;
	}

	~Shader();
	
	static std::optional<Shader> compileShader(std::string source, ShaderType type);
	inline uint32_t getId() const { return m_id; };

private:
	Shader() = default;

	std::string m_source;
	uint32_t m_id = 0;
};

class ShaderProgram {
public:
	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram(ShaderProgram&& other) {
		this->m_id = other.m_id;
		other.m_id = 0;
	}
	ShaderProgram& operator=(const ShaderProgram&) = delete;
	ShaderProgram& operator=(ShaderProgram&& other) {
		this->m_id = other.m_id;
		other.m_id = 0;
		return *this;
	};
	~ShaderProgram();

	static std::optional<ShaderProgram> createAndLink(const std::vector<Shader*>& shaders);

	void bind();

	inline uint32_t getId() const { return m_id; }

private:
	ShaderProgram() = default;
	uint32_t m_id;

};



