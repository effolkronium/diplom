#pragma once

#include <string>
#include <filesystem>

namespace utils
{
	std::string readFile(std::filesystem::path path);
}