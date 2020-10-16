#pragma once

class IRender
{
public:
	virtual ~IRender() = default;

	virtual void startRenderLoop() = 0;
};