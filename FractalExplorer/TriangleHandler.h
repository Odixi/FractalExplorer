#pragma once

#include "utils.h"

using Color = glm::vec4;

struct Vertex {
	glm::vec2 pos;
	Color color;
};

struct TriangleInfo {
	double cost;
	int neighbors[3];
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

	// start index
	double calculateTriangleCost(uint32_t index);

	void validateTriangleNegihbors();

	VertexGenerator m_vertexGenerator;

	double m_scale = 2;
	int m_maxIterations = 300;

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;

	std::vector<int> m_nrVertRef; // Number of trianlges a vertex refers to
	std::vector<uint32_t> m_freeEntries; // array of indices that are free in m_vertices

	std::vector<TriangleInfo> m_triangleInfos;
};

