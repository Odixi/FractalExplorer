#pragma once

namespace mandelbrot {

	template<typename NumericType>
	constexpr std::complex<NumericType> calculateNext(std::complex<NumericType> z, std::complex<NumericType> c) {
		return std::pow(z, static_cast<NumericType>(2)) + c;
	}

	template<typename NumericType>
	constexpr std::pair<int, std::complex<NumericType>> calculateEscapeTime(std::complex<NumericType> start, int maxIter) {
		std::complex<NumericType> z = start;
		int n = 0;
		while (std::norm(z) < 16 && n < maxIter) {
			z = calculateNext(z, start);
			++n;
		}
		return { n, z };
	}

	template<typename NumericType>
	constexpr double calculateSmoothEscapeTime(std::complex<NumericType> start, int maxIter) {
		auto result = calculateEscapeTime(start, maxIter);
		return result.first < maxIter
			? result.first - std::clamp(std::log(std::log(std::abs(result.second))) / std::log(2.0), 0.0, 1.0)
			: static_cast<double>(maxIter);
	}
}