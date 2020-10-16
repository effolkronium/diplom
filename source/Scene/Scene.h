#include "IRender.h"

class Scene
{
public:
	Scene(IRender& render);

	virtual void run() = 0;
private:
	IRender& m_render;
};