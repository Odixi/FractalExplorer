#include "pch.h"

#include "utils.h"

namespace utils {

	std::string readTextFile(const std::string& path)
	{
		std::ifstream stream{ path.c_str()};
		return std::string((std::istreambuf_iterator<char>(stream)),
						(std::istreambuf_iterator<char>()));
	}
}
