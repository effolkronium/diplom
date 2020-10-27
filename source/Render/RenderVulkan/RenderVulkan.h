#pragma once

#include "Render.h"
#include <memory>

class RenderVulkan : public IRender
{
public:
	RenderVulkan();
	~RenderVulkan();

	void startRenderLoop(std::vector<ModelInfo> modelInfos) override;
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};