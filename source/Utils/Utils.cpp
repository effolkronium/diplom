#include "Utils.h"
#include <fstream>

namespace fs = std::filesystem;
using namespace std::string_literals;

namespace utils
{
	std::string readFile(std::filesystem::path path)
	{
		if (!fs::exists(path))
			throw std::runtime_error{ "readFile path not found: " +
				fs::absolute(path).string() };
		
		std::ifstream t(path);
		t.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		t.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		t.seekg(0, std::ios::end);
		size_t size = t.tellg();
		std::string buffer(size, ' ');
		t.seekg(0);
		t.read(&buffer[0], size);

		return buffer;
	}
}