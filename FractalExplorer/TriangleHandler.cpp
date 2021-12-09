#include "pch.h"

#include "TriangleHandler.h"

void TriangleHandler::generateVertices(int amount)
{
	if (m_vertices.empty()) {
		generateInitialVertices();
	}

	for (int i = 0; i < amount; ++i) {
		if (m_vertices.size() >= constants::maxVertices) {
			return;
		}
		//const uint32_t randomIndex = rand() % (m_indices.size() / 3);
		const auto max = std::ranges::max_element(m_triangleCosts);
		const uint32_t index = std::distance(m_triangleCosts.begin(), max);

		divideTriangle(index * 3);
	}
}

void TriangleHandler::generateInitialVertices()
{
	m_vertices.reserve(1000000);
	m_indices.reserve(1000000);

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

	m_triangleCosts.push_back(calculateTriangleCost(0));
	m_triangleCosts.push_back(calculateTriangleCost(3));
}

void TriangleHandler::divideTriangle(uint32_t index)
{
	const uint32_t ti0 = m_indices[index];
	const uint32_t ti1 = m_indices[index+1];
	const uint32_t ti2 = m_indices[index+2];

	const auto getMiddle = [&](uint32_t i1, uint32_t i2) -> glm::vec2 {
		return (m_vertices[i1].pos + m_vertices[i2].pos) / 2.0f;
	};

	// Hypotenusa
	uint32_t hi0, hi1, tip;

	const auto squaredDistance = [](const glm::vec2& a, const glm::vec2& b	) {
		const auto d = (a - b);
		return glm::dot(d, d);
	};
	
	float l0 = squaredDistance(m_vertices[ti0].pos, m_vertices[ti1].pos);
	float l1 = squaredDistance(m_vertices[ti1].pos, m_vertices[ti2].pos);
	float l2 = squaredDistance(m_vertices[ti0].pos, m_vertices[ti2].pos);
	if (l0 > l1 && l0 > l2) {
		hi0 = ti0;
		hi1 = ti1;
		tip = ti2;
	}
	if (l1 > l2) {
		hi0 = ti1;
		hi1 = ti2;
		tip = ti0;
	}
	else {
		hi0 = ti0;
		hi1 = ti2;
		tip = ti1;
	}
	auto middle = getMiddle(hi0, hi1);
	const uint32_t newIndex = m_vertices.size();
	m_vertices.push_back(m_vertexGenerator(middle, m_scale, m_maxIterations));

	const auto slice = [&](uint32_t originalStartIndex, uint32_t h0, uint32_t h1, uint32_t t) ->void {
		// Use the old triangle slot
		m_indices[originalStartIndex] = newIndex;
		m_indices[originalStartIndex + 1] = h0;
		m_indices[originalStartIndex + 2] = t;
		m_triangleCosts[originalStartIndex / 3] = calculateTriangleCost(originalStartIndex);

		const uint32_t newTriIndex = m_indices.size();
		m_indices.push_back(newIndex);
		m_indices.push_back(t);
		m_indices.push_back(h1);
		m_triangleCosts.push_back(calculateTriangleCost(newTriIndex));
	};

	slice(index, hi0, hi1, tip);


	// Find the other triangle with the edge
	for (int i = 0; i < m_indices.size(); i += 3) {
		if ((m_indices[i] == hi0 || m_indices[i + 1] == hi0 || m_indices[i + 2] == hi0) 
			&& (m_indices[i] == hi1 || m_indices[i + 1] == hi1 || m_indices[i + 2] == hi1)) {
			int otherTip;
			if (m_indices[i] != hi0 && m_indices[i] != hi1) {
				otherTip = m_indices[i];
			}
			else if (m_indices[i+1] != hi0 && m_indices[i+1] != hi1) {
				otherTip = m_indices[i+1];
			}
			else {
				otherTip = m_indices[i + 2];
			}
			slice(i, hi0, hi1, otherTip);
			return;
		}
	}
	int a = 0;
	return;
}

double TriangleHandler::calculateTriangleCost(uint32_t index)
{
	const Vertex& v0 = m_vertices[m_indices[index]];
	const Vertex& v1 = m_vertices[m_indices[index+1]];
	const Vertex& v2 = m_vertices[m_indices[index+2]];

	const auto squaredDistance = [](const glm::vec2& a, const glm::vec2& b) {
		const auto d = (a - b);
		return glm::dot(d, d);
	};

	const double colorDiff = squaredDistance(v0.color, v1.color)
					  + squaredDistance(v1.color, v2.color)
						+ squaredDistance(v0.color, v2.color);

	double lenghts[3];

	lenghts[0] = squaredDistance(v0.pos, v1.pos);
	lenghts[1] = squaredDistance(v1.pos, v2.pos);
	lenghts[2] = squaredDistance(v0.pos, v2.pos);
	
	const double totalSideLen = lenghts[0] + lenghts[1] + lenghts[2];

	const double slRatio = std::ranges::max(lenghts) / std::ranges::min(lenghts);


	return (0.00001+colorDiff) * ((totalSideLen*slRatio));
}
