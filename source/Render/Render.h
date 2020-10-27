#pragma once

#include <string>
#include <vector>

struct ModelInfo
{
	float posX{}, posY{}, posZ{};
	float scaleX{1.f}, scaleY{1.f}, scaleZ{1.f};

	std::string modelPath;
	std::string texturePath;
};


class IRender
{
public:
	virtual ~IRender() = default;

	virtual void startRenderLoop(std::vector<ModelInfo> modelInfos) = 0;
};