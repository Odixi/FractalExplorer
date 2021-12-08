#include "pch.h"

#include "TriangleHandler.h"

void TriangleHandler::generateVertices(int amount)
{
	if (m_vertices.empty()) {
		generateInitialVertices();
	}

	for (int i = 0; i < amount; i += 3) {
		if (m_vertices.size() >= constants::maxVertices) {
			return;
		}
		const uint32_t randomIndex = rand() % (m_indices.size() / 3);
		divideTriangle(randomIndex * 3);
	}
}

void TriangleHandler::generateInitialVertices()
{
	m_vertices = std::vector<Vertex>{
		m_vertexGenerator({1,1}, m_scale, m_maxIterations),
		m_vertexGenerator({-1,1}, m_scale, m_maxIterations),
		m_vertexGenerator({-1,-1}, m_scale, m_maxIterations),
		m_vertexGenerator({1,-1}, m_scale, m_maxIterations),
	};

	m_indices = std::vector<uint32_t>{
		0,1,2,
		2,3,0
	};
}

void TriangleHandler::divideTriangle(uint32_t index)
{
	const uint32_t op0 = m_indices[index];
	const uint32_t op1 = m_indices[index+1];
	const uint32_t op2 = m_indices[index+2];

	const auto getPos = [&](uint32_t i1, uint32_t i2) -> glm::vec2 {
		return (m_vertices[i1].pos + m_vertices[i2].pos) / 2.0f;
	};

	const uint32_t p0 = m_vertices.size();
	const uint32_t p1 = p0 + 1;
	const uint32_t p2 = p0 + 2;
	m_vertices.push_back(m_vertexGenerator(getPos(op0, op1), m_scale, m_maxIterations));
	m_vertices.push_back(m_vertexGenerator(getPos(op1, op2), m_scale, m_maxIterations));
	m_vertices.push_back(m_vertexGenerator(getPos(op0, op2), m_scale, m_maxIterations));

	// The first triangle can take the plcae of the old
	m_indices[index] = p0;
	m_indices[index+1] = p1;
	m_indices[index+2] = p2;

	m_indices.push_back(op0);
	m_indices.push_back(p0);
	m_indices.push_back(p2);

	m_indices.push_back(op1);
	m_indices.push_back(p1);
	m_indices.push_back(p0);

	m_indices.push_back(p1);
	m_indices.push_back(op2);
	m_indices.push_back(p2);
}
