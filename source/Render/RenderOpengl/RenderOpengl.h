#pragma once

#include "Render.h"
#include <memory>


class RenderOpengl : public IRender
{
public:
	RenderOpengl();
	~RenderOpengl();

	double startRenderLoop(std::vector<ModelInfo> modelInfos) override;
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};