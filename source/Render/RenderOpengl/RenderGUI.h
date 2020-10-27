#pragma once

#include "Render.h"
#include <memory>


struct RenderGuiData
{
	enum class RenderType
	{
		OpenGL, Vulkan, Exit
	} renderType;

	enum class SceneLoad
	{
		Low, Med, High
	} sceneLoad;
};


class RenderGUI
{
public:
	RenderGUI();
	~RenderGUI();

	RenderGuiData startRenderLoop();
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};