#pragma once

#include "IRender.h"

class RenderVulkan : public IRender
{
public:
	RenderVulkan();

	void startRenderLoop() override;
};