#pragma once

#include "Render.h"
#include <memory>


class RenderGUI
{
public:
	RenderGUI();
	~RenderGUI();

	RenderGuiData startRenderLoop(RenderGuiData result);
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};