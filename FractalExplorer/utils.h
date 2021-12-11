#pragma once

namespace utils {

	std::string readTextFile(const std::string& path);

}

namespace geom {

	struct BBox2 {

		glm::vec2 minPoint{ 0,0 };
		glm::vec2 maxPoint{ 0,0 };

		BBox2(){};
		BBox2(const glm::vec2& p1, const glm::vec2 p2) {
			minPoint.x = glm::min(p1.x, p2.x);
			minPoint.y = glm::min(p1.y, p2.y);
			maxPoint.x = glm::max(p1.x, p2.x);
			maxPoint.y = glm::max(p1.y, p2.y);
		}
		BBox2(std::initializer_list<glm::vec2> l) {
			if (l.size() > 0) {
				minPoint = *l.begin();
				maxPoint = *l.begin();
				expandToContain(l);
			}
		}

		template<typename Container>
		void expandToContain(const Container& points) {
			for (const glm::vec2& p : points) {
				expandToContain(p);
			}
		}

		constexpr void expandToContain(const glm::vec2& point) {
			minPoint.x = glm::min(minPoint.x, point.x);
			minPoint.y = glm::min(minPoint.y, point.y);
			maxPoint.x = glm::max(maxPoint.x, point.x);
			maxPoint.y = glm::max(maxPoint.y, point.y);
		}

		constexpr bool containsPoint(const glm::vec2& point) const {
			return point.x >= minPoint.x && point.y >= minPoint.y &&
				point.x <= maxPoint.x && point.y <= maxPoint.y;
		}

		template<typename Container>
		bool containsAll(const Container& points) const{
			return std::ranges::all_of(points, [&](const glm::vec2& p) {return containsPoint(p); });
		}

		template<typename Container>
		bool containsAny(const Container& points) const {
			return std::ranges::any_of(points, [&](const glm::vec2& p) {return containsPoint(p); });
		}

		template<typename Container>
		bool containsNone(const Container& points) const {
			return std::ranges::none_of(points, [&](const glm::vec2& p) {return containsPoint(p); });
		}

		constexpr glm::vec2 center() const {
			return (minPoint + maxPoint) / 2.0f;
		}
	};
}