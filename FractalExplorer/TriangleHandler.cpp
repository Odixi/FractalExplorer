#include "pch.h"

#include "TriangleHandler.h"

void TriangleHandler::generateVertices(const geom::BBox2& screenBb, int amount)
{
	if (m_vertices.empty() || m_indices.empty()) {
		generateInitialVertices();
	}

	const int slices = m_triangleInfos.size() / amount;

	for (int i = 0; i < amount; ++i) {
		if (m_vertices.size()-m_freeEntries.size() >= constants::maxVertices) {
			return;
		}
		//const uint32_t randomIndex = rand() % (m_indices.size() / 3);
		const auto start = m_triangleInfos.begin() + (i * slices);
		const auto end = start + slices;

		const auto max = std::ranges::max_element(start, end, [](const TriangleInfo& a, const TriangleInfo& b) {return a.cost < b.cost; });
		uint32_t index = std::distance(m_triangleInfos.begin(), max);
		
		uint32_t triangleIndex = index * 3;
		auto p1 = m_indices[triangleIndex];
		auto p2 = m_indices[triangleIndex + 1];
		auto p3 = m_indices[triangleIndex + 2];
		glm::vec2 triangle[3] = { m_vertices[p1].pos, m_vertices[p2].pos, m_vertices[p3].pos };
		if (!screenBb.containsAny(triangle)) {
			m_triangleInfos[index].cost -= 1;
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
		if (!screenBb.collidesWith({ m_vertices[p1].pos, m_vertices[p2].pos, m_vertices[p3].pos })) {
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
		auto& toRemoveTriangle = m_triangleInfos[indexToRemove / 3];
		auto& toSwapWithTrianle = m_triangleInfos[(lastIndex-3) / 3];
		for (auto& neighbor : toRemoveTriangle.neighbors) {
			if (neighbor < 0) {
				continue;
			}
			for (auto& nn : m_triangleInfos[neighbor].neighbors) {
				if (nn == (indexToRemove / 3)) {
					nn = -1;
				}
			}
		}
		if (indexToRemove != lastIndex - 1) {
			for (auto& neighbor : toSwapWithTrianle.neighbors) {
				if (neighbor < 0) {
					continue;
				}
				for (auto& nn : m_triangleInfos[neighbor].neighbors) {
					if (nn == ((lastIndex-3 ) / 3)) {
						nn = indexToRemove/3;
					}
				}
			}
		}
		m_nrVertRef[m_indices[indexToRemove]]--;
		m_nrVertRef[m_indices[indexToRemove+1]]--;
		m_nrVertRef[m_indices[indexToRemove+2]]--;
		m_indices[indexToRemove + 2] = m_indices[--lastIndex];
		m_indices[indexToRemove + 1] = m_indices[--lastIndex];
		m_indices[indexToRemove] = m_indices[--lastIndex];

		m_triangleInfos[indexToRemove / 3] = m_triangleInfos[lastIndex / 3];
	}
	m_indices.erase(m_indices.begin() + lastIndex, m_indices.end());
	m_triangleInfos.erase(m_triangleInfos.begin() + lastIndex /3, m_triangleInfos.end());

	std::ranges::sort(verticesToRemove);
	auto [first, last] = std::ranges::unique(verticesToRemove);
	verticesToRemove.erase(first, last);

	// Mark as free
	for (const auto& index : verticesToRemove) {
		if (m_nrVertRef[index] == 0) {
			m_freeEntries.push_back(index);
		}
	}

}

void TriangleHandler::generateInitialVertices()
{
	m_vertices.reserve(constants::maxVertices);
	m_nrVertRef.reserve(constants::maxVertices);
	m_indices.reserve(constants::maxVertices*3);
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



	m_triangleInfos.push_back(TriangleInfo{
		.cost = calculateTriangleCost(0),
		.neighbors = {-1, -1, 1}
	});
	m_triangleInfos.push_back(TriangleInfo{
		.cost = calculateTriangleCost(3),
		.neighbors = {-1, -1, 0}
		});

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

	int h0h1Neighbor;
	int h0TipNeighbor;
	int h1TipNeighbor;
	if (l0 > l1 && l0 > l2) {
		hi0 = ti0;
		hi1 = ti1;
		tip = ti2;
		h0h1Neighbor = m_triangleInfos[index / 3].neighbors[0];
		h0TipNeighbor = m_triangleInfos[index / 3].neighbors[2];
		h1TipNeighbor = m_triangleInfos[index / 3].neighbors[1];
	}
	if (l1 > l2) {
		hi0 = ti1;
		hi1 = ti2;
		tip = ti0;
		h0h1Neighbor = m_triangleInfos[index / 3].neighbors[1];
		h0TipNeighbor = m_triangleInfos[index / 3].neighbors[0];
		h1TipNeighbor = m_triangleInfos[index / 3].neighbors[2];
	}
	else {
		hi0 = ti0;
		hi1 = ti2;
		tip = ti1;
		h0h1Neighbor = m_triangleInfos[index / 3].neighbors[2];
		h0TipNeighbor = m_triangleInfos[index / 3].neighbors[0];
		h1TipNeighbor = m_triangleInfos[index / 3].neighbors[1];
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

	const auto updateNeigbors = [&](int triangleToUpdate, int oldIndex, int newIndex) {
		// Update the neighbors
		if (triangleToUpdate >= 0) {
			auto& update = m_triangleInfos[triangleToUpdate];
			for (auto& n : update.neighbors) {
				if (n == oldIndex) {
					n = newIndex;
				}
			}
		}
	};

	// Slice the (possibly) two triangles
	const uint32_t newTriIndex = m_indices.size();

	// Use the old triangle slot
	m_indices[index] = newIndex;
	m_indices[index + 1] = hi0;
	m_indices[index + 2] = tip;
	m_triangleInfos[index / 3] = TriangleInfo{
		.cost = calculateTriangleCost(index),
		.neighbors = { 
			h0h1Neighbor , 
			h0TipNeighbor, 
			static_cast<int>(newTriIndex/3)}
	};


	m_indices.push_back(newIndex);
	m_indices.push_back(tip);
	m_indices.push_back(hi1);
	m_triangleInfos.push_back(TriangleInfo{
		.cost = calculateTriangleCost(newTriIndex),
		.neighbors = {
			static_cast<int>(index/3), 
			h1TipNeighbor, 
			h0h1Neighbor >= 0 ? static_cast<int>(newTriIndex / 3) + 1 : -1}
	});


	// Update the neighbors
	updateNeigbors(h1TipNeighbor, index/3, newTriIndex/3);

	// Add vertex refs
	m_nrVertRef[tip]++;
	m_nrVertRef[newIndex] += 2;

	if (h0h1Neighbor < 0) {
		return;
	}

	// Handle the neighbour triangle
	auto otherTriangle = m_triangleInfos[h0h1Neighbor];
	int otherTriangleIndex = h0h1Neighbor * 3;

	int otherTip, otherH1Neighbor, otherH0Neighbor;
	if (m_indices[otherTriangleIndex] != hi0 && m_indices[otherTriangleIndex] != hi1) {
		otherTip = m_indices[otherTriangleIndex];
		if (m_indices[otherTriangleIndex + 1] == hi0) {
			otherH0Neighbor = otherTriangle.neighbors[0];
			otherH1Neighbor = otherTriangle.neighbors[2];
		}
		else {
			assert((m_indices[otherTriangleIndex + 2]) == hi0);
			otherH0Neighbor = otherTriangle.neighbors[2];
			otherH1Neighbor = otherTriangle.neighbors[0];
		}
	}
	else if (m_indices[otherTriangleIndex+1] != hi0 && m_indices[otherTriangleIndex+1] != hi1) {
		otherTip = m_indices[otherTriangleIndex + 1];
		if (m_indices[otherTriangleIndex + 2] == hi0) {
			otherH0Neighbor = otherTriangle.neighbors[1];
			otherH1Neighbor = otherTriangle.neighbors[0];
		}
		else {
			assert((m_indices[otherTriangleIndex]) == hi0);
			otherH0Neighbor = otherTriangle.neighbors[0];
			otherH1Neighbor = otherTriangle.neighbors[1];
		}
	}
	else {
		otherTip = m_indices[otherTriangleIndex + 2];
		if (m_indices[otherTriangleIndex] == hi0) {
			otherH0Neighbor = otherTriangle.neighbors[2];
			otherH1Neighbor = otherTriangle.neighbors[1];
		}
		else {
			assert((m_indices[otherTriangleIndex+1]) == hi0);
			otherH0Neighbor = otherTriangle.neighbors[1];
			otherH1Neighbor = otherTriangle.neighbors[2];
		}
	}

	// Use the old triangle slot
	m_indices[otherTriangleIndex] = newIndex;
	m_indices[otherTriangleIndex + 1] = hi0;
	m_indices[otherTriangleIndex + 2] = otherTip;
	m_triangleInfos[otherTriangleIndex/3] = TriangleInfo{
		.cost = calculateTriangleCost(otherTriangleIndex),
		.neighbors = {
			static_cast<int> (index/3),
			otherH0Neighbor, 
			static_cast<int>(newTriIndex/3)+1}
	};


	m_indices.push_back(newIndex);
	m_indices.push_back(otherTip);
	m_indices.push_back(hi1);
	m_triangleInfos.push_back(TriangleInfo{
		.cost = calculateTriangleCost(newTriIndex+3),
		.neighbors = {
			static_cast<int>(otherTriangleIndex/3), 
			otherH1Neighbor, 
			static_cast<int>(newTriIndex/3)}
		});

	// Add vertex refs
	m_nrVertRef[otherTip]++;
	m_nrVertRef[newIndex] += 2;

	// Update the neighbors
	updateNeigbors(otherH1Neighbor, otherTriangleIndex/3, newTriIndex/3 + 1);

	//validateTriangleNegihbors();
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


	return (0.0001+colorDiff) * ((totalSideLen/**slRation*/));
}

void TriangleHandler::validateTriangleNegihbors()
{
	for (int t = 0; t < m_indices.size(); t += 3) {

		int t1 = m_indices[t], t2 = m_indices[t + 1], t3 = m_indices[t + 2];
		auto& triangleInfo = m_triangleInfos[t / 3];

		for (int e = 0; e < 3; ++e) {
			int e1, e2;
			if (e == 0) {
				e1 = t1;
				e2 = t2;
			}
			if (e == 1) {
				e1 = t2;
				e2 = t3;
			}
			if (e == 2) {
				e1 = t3;
				e2 = t1;
			}

			int neighbor = -1;

			// Find the other triangle with the edge
			for (int i = 0; i < m_indices.size(); i += 3) {
				if ((m_indices[i] == e1 || m_indices[i + 1] == e1 || m_indices[i + 2] == e1)
					&& (m_indices[i] == e2 || m_indices[i + 1] == e2 || m_indices[i + 2] == e2) && i != t) {
					
					neighbor = i/3;
				}
			}

			assert(neighbor == triangleInfo.neighbors[e]);
		}
	}
}
