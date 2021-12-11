#pragma once

#include "utils.h"

using Color = glm::vec4;

struct Vertex {
	glm::vec2 pos;
	Color color;
};

using VertexGenerator = std::function<Vertex(glm::vec2, double, int)>;

namespace constants {
	constexpr size_t maxVertices = 100000;
}

class TriangleHandler
{
public:
	TriangleHandler(const VertexGenerator vgen):m_vertexGenerator(vgen) {}

	void generateVertices(const geom::BBox2& screenBb, int amount);
	void removeTrianglesOutsideScreen(const geom::BBox2& screenBb, int maxToRemove);

	const std::vector<Vertex>& getVertices() const { return m_vertices; }
	const std::vector<uint32_t>& getIndeices() const { return m_indices; }

private:
	void generateInitialVertices();
	void divideTriangle(uint32_t index);

	// Note: triangles should already be removed!
	void removeVerices(const std::vector<uint32_t>& vertexIndices);

	double calculateTriangleCost(uint32_t index);

	VertexGenerator m_vertexGenerator;

	double m_scale = 2;
	int m_maxIterations = 400;

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;

	std::vector<uint32_t> m_freeEntries;

	std::vector<double> m_triangleCosts;
};

