#pragma once

#include <string>
#include <vector>

struct RenderGuiData
{
	RenderGuiData() {}
	enum class RenderType
	{
		OpenGL, Vulkan, Exit
	} renderType{ RenderType::OpenGL };

	enum class SceneLoad
	{
		Low, Med, High
	} sceneLoad{ SceneLoad::Med };

	int modelNumber = 10;

	double averageFps = -1;

	bool simpleScene = false;
};

struct ModelInfo
{
	float posX{}, posY{}, posZ{};
	float scaleX{1.f}, scaleY{1.f}, scaleZ{1.f};

	std::string modelPath;
	std::string texturePath;

	int animationNumber = 0;
	int maxAnimationNumber = 0;

	bool simpleModel = true;
};


class IRender
{
public:
	virtual ~IRender() = default;

	virtual double startRenderLoop(std::vector<ModelInfo> modelInfos) = 0;
};