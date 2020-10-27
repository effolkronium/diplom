#include "Render.h"

class Scene
{
public:
	Scene(IRender& render);

	void run();
private:
	IRender& m_render;

	std::vector<ModelInfo> m_modelInfos;
};