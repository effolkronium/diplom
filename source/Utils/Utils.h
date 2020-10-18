#pragma once

#include <string>
#include <filesystem>
#include <numeric>

namespace utils
{
	std::string readFile(std::filesystem::path path);


	template<typename NumL, typename NumR>
	NumL intCast(NumR num)
	{
		if (num > std::numeric_limits<NumL>::max())
			throw std::runtime_error{ "intCast error" };

		return static_cast<NumL>(num);
	}
}