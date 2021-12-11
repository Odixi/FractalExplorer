#include "pch.h"

#include "TriangleHandler.h"

void TriangleHandler::generateVertices(const geom::BBox2& screenBb, int amount)
{
	if (m_vertices.empty() || m_indices.empty()) {
		generateInitialVertices();
	}

	const int slices = m_triangleCosts.size() / amount;

	for (int i = 0; i < amount; ++i) {
		if (m_vertices.size()-m_freeEntries.size() >= constants::maxVertices) {
			return;
		}
		//const uint32_t randomIndex = rand() % (m_indices.size() / 3);
		const auto start = m_triangleCosts.begin() + (i * slices);
		const auto end = start + slices;

		const auto max = std::ranges::max_element(start, end);
		uint32_t index = std::distance(m_triangleCosts.begin(), max);
		
		uint32_t triangleIndex = index * 3;
		auto p1 = m_indices[triangleIndex];
		auto p2 = m_indices[triangleIndex + 1];
		auto p3 = m_indices[triangleIndex + 2];
		glm::vec2 triangle[3] = { m_vertices[p1].pos, m_vertices[p2].pos, m_vertices[p3].pos };
		if (!screenBb.containsAny(triangle)) {
			m_triangleCosts[index] -= 1;
			continue;
		}

		divideTriangle(triangleIndex);
	}
}

void TriangleHandler::removeTrianglesOutsideScreen(const geom::BBox2& screenBb, int maxToRemove)
{
	std::vector<uint32_t> indicesToRemove;
	std::vector<uint32_t> verticesToRemove; // There are not neccessarily removed
	for (uint32_t i = 0; i < m_indices.size(); i += 3) {
		auto p1 = m_indices[i];
		auto p2 = m_indices[i+1];
		auto p3 = m_indices[i+2];
		glm::vec2 triangle[3] = {m_vertices[p1].pos, m_vertices[p2].pos, m_vertices[p3].pos };
		if (!screenBb.containsAny(triangle)) {
			indicesToRemove.push_back(i);
			verticesToRemove.push_back(p1);
			verticesToRemove.push_back(p2);
			verticesToRemove.push_back(p3);
			if (indicesToRemove.size() >= maxToRemove) {
				break;
			}
		}
	}

	if (indicesToRemove.empty()) {
		return;
	}

	// The indices are quaranteed to be in order
	// Go backwars so that removing doesn't 'shift' the other indices
	size_t lastIndex = m_indices.size();
	for (int i = indicesToRemove.size() - 1; i >= 0; --i) {
		const int indexToRemove = indicesToRemove[i];
		m_nrVertRef[m_indices[indexToRemove]]--;
		m_nrVertRef[m_indices[indexToRemove+1]]--;
		m_nrVertRef[m_indices[indexToRemove+2]]--;
		m_indices[indexToRemove + 2] = m_indices[--lastIndex];
		m_indices[indexToRemove + 1] = m_indices[--lastIndex];
		m_indices[indexToRemove] = m_indices[--lastIndex];
		m_triangleCosts[indexToRemove / 3] = m_triangleCosts[lastIndex / 3];
	}
	m_indices.erase(m_indices.begin() + lastIndex, m_indices.end());
	m_triangleCosts.erase(m_triangleCosts.begin() + lastIndex /3, m_triangleCosts.end());

	std::ranges::sort(verticesToRemove);
	auto [first, last] = std::ranges::unique(verticesToRemove);
	verticesToRemove.erase(first, last);

	// Remove if it is still reference by some triangles
	//std::erase_if(verticesToRemove, 
	//	[&](const uint32_t& removed) {
	//	return std::ranges::any_of(m_indices, 
	//		[&](const uint32_t& current) {
	//		return removed == current; 
	//	}); 
	//});

	// Mark as free
	for (const auto& index : verticesToRemove) {
		if (m_nrVertRef[index] == 0) {
			m_freeEntries.push_back(index);
		}
	}
	
	//m_freeEntries.insert(m_freeEntries.begin(), verticesToRemove.begin(), verticesToRemove.end());

	//removeVerices(verticesToRemove);
}

void TriangleHandler::generateInitialVertices()
{
	m_vertices.reserve(constants::maxVertices);
	m_indices.reserve(constants::maxVertices);
	m_freeEntries.reserve(constants::maxVertices);

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

	m_nrVertRef = std::vector<int>{
		2, 1, 2, 1
	};
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

	uint32_t newIndex;
	if (m_freeEntries.empty()) {
		newIndex = m_vertices.size();
		m_vertices.push_back(m_vertexGenerator(middle, m_scale, m_maxIterations));
		m_nrVertRef.push_back(0);
	}
	else {
		newIndex = m_freeEntries.back();
		m_freeEntries.pop_back();
		m_vertices[newIndex] = m_vertexGenerator(middle, m_scale, m_maxIterations);
	}

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

		// Add vertex refs
		m_nrVertRef[t]++;
		m_nrVertRef[newIndex] += 2;
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

void TriangleHandler::removeVerices(const std::vector<uint32_t>& vertexIndices)
{
	for (int i = static_cast<int>(vertexIndices.size()) - 1; i >= 0; --i) {
		// Swap the vertex to the end, erase it and update the swapped vertex indices
		const uint32_t index = vertexIndices[i];
		const uint32_t last = m_vertices.size() - 1;
		m_vertices[index] = m_vertices[last];
		for (uint32_t j = 0; j < m_indices.size(); ++j) {
			if (m_indices[j] == last) {
				m_indices[j] = index;
			}
		}
		m_vertices.pop_back();
	}
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

	//const double slRatio = std::ranges::max(lenghts) / std::ranges::min(lenghts);


	return (0.000001+colorDiff) * ((totalSideLen/**slRation*/));
}
