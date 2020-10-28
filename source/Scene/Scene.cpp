#include "Scene.h"
#include <array>
#include "glm/glm/glm.hpp"

namespace
{
	int fixedRundom[]{
		3,	3,	2,	3,
		3,	3,	1,	3,
		0,	3,	0,	3,
		2,	3,	0,	3,
		3,	3,	1,	0,
		3,	0,	3,	1,
		2,	1,	3,	0,
		1,	3,	2,	3,
		3,	3,	1,	2,
		3,	3,	2,	2,
		3,	1,	0,	1,
		3,	3,	3,	2,
		3,	0,	1,	3,
		3,	3,	0,	3,
		0,	3,	0,	2,
		3,	2,	2,	0,
		3,	0,	0,	2,
		3,	3,	0,	1,
		3,	3,	2,	3,
		1,	0,	3,	3,
		1,	3,	1,	1,
		2,	0,	2,	3,
		3,	3,	1,	0,
		3,	3,	1,	0,
		1,	0,	0,	1,
		1,	0,	2,	1,
		0,	1,	0,	3,
		1,	3,	0,	1,
		1,	0,	0,	2,
		3,	1,	2,	1,
		3,	3,	2,	3,
		3,	3,	1,	3,
		0,	3,	0,	3,
		2,	3,	0,	3,
		3,	3,	1,	0,
		3,	0,	3,	1,
		2,	1,	3,	0,
		1,	3,	2,	3,
		3,	3,	1,	2,
		3,	3,	2,	2,
		3,	1,	0,	1,
		3,	3,	3,	2,
		3,	0,	1,	3,
		3,	3,	0,	3,
		0,	3,	0,	2,
		3,	2,	2,	0,
		3,	0,	0,	2,
		3,	3,	0,	1,
		3,	3,	2,	3,
		1,	0,	3,	3,
		1,	3,	1,	1,
		2,	0,	2,	3,
		3,	3,	1,	0,
		3,	3,	1,	0,
		1,	0,	0,	1,
		1,	0,	2,	1,
		0,	1,	0,	3,
		1,	3,	0,	1,
		1,	0,	0,	2,
		3,	1,	2,	1,
		3,	3,	2,	3,
		3,	3,	1,	3,
		0,	3,	0,	3,
		2,	3,	0,	3,
		3,	3,	1,	0,
		3,	0,	3,	1,
		2,	1,	3,	0,
		1,	3,	2,	3,
		3,	3,	1,	2,
		3,	3,	2,	2,
		3,	1,	0,	1,
		3,	3,	3,	2,
		3,	0,	1,	3,
		3,	3,	0,	3,
		0,	3,	0,	2,
		3,	2,	2,	0,
		3,	0,	0,	2,
		3,	3,	0,	1,
		3,	3,	2,	3,
		1,	0,	3,	3,
		1,	3,	1,	1,
		2,	0,	2,	3,
		3,	3,	1,	0,
		3,	3,	1,	0,
		1,	0,	0,	1,
		1,	0,	2,	1,
		0,	1,	0,	3,
		1,	3,	0,	1,
		1,	0,	0,	2,
		3,	1,	2,	1,
		3,	3,	2,	3,
		3,	3,	1,	3,
		0,	3,	0,	3,
		2,	3,	0,	3,
		3,	3,	1,	0,
		3,	0,	3,	1,
		2,	1,	3,	0,
		1,	3,	2,	3,
		3,	3,	1,	2,
		3,	3,	2,	2,
		3,	1,	0,	1,
		3,	3,	3,	2,
		3,	0,	1,	3,
		3,	3,	0,	3,
		0,	3,	0,	2,
		3,	2,	2,	0,
		3,	0,	0,	2,
		3,	3,	0,	1,
		3,	3,	2,	3,
		1,	0,	3,	3,
		1,	3,	1,	1,
		2,	0,	2,	3,
		3,	3,	1,	0,
		3,	3,	1,	0,
		1,	0,	0,	1,
		1,	0,	2,	1,
		0,	1,	0,	3,
		1,	3,	0,	1,
		1,	0,	0,	2,
		3,	1,	2,	1,
		3,	3,	2,	3,
		3,	3,	1,	3,
		0,	3,	0,	3,
		2,	3,	0,	3,
		3,	3,	1,	0,
		3,	0,	3,	1,
		2,	1,	3,	0,
		1,	3,	2,	3,
		3,	3,	1,	2,
		3,	3,	2,	2,
		3,	1,	0,	1,
		3,	3,	3,	2,
		3,	0,	1,	3,
		3,	3,	0,	3,
		0,	3,	0,	2,
		3,	2,	2,	0,
		3,	0,	0,	2,
		3,	3,	0,	1,
		3,	3,	2,	3,
		1,	0,	3,	3,
		1,	3,	1,	1,
		2,	0,	2,	3,
		3,	3,	1,	0,
		3,	3,	1,	0,
		1,	0,	0,	1,
		1,	0,	2,	1,
		0,	1,	0,	3,
		1,	3,	0,	1,
		1,	0,	0,	2,
		3,	1,	2,	1,
		3,	3,	2,	3,
		3,	3,	1,	3,
		0,	3,	0,	3,
		2,	3,	0,	3,
		3,	3,	1,	0,
		3,	0,	3,	1,
		2,	1,	3,	0,
		1,	3,	2,	3,
		3,	3,	1,	2,
		3,	3,	2,	2,
		3,	1,	0,	1,
		3,	3,	3,	2,
		3,	0,	1,	3,
		3,	3,	0,	3,
		0,	3,	0,	2,
		3,	2,	2,	0,
		3,	0,	0,	2,
		3,	3,	0,	1,
		3,	3,	2,	3,
		1,	0,	3,	3,
		1,	3,	1,	1,
		2,	0,	2,	3,
		3,	3,	1,	0,
		3,	3,	1,	0,
		1,	0,	0,	1,
		1,	0,	2,	1,
		0,	1,	0,	3,
		1,	3,	0,	1,
		1,	0,	0,	2,
		3,	1,	2,	1,
		3,	3,	2,	3,
		3,	3,	1,	3,
		0,	3,	0,	3,
		2,	3,	0,	3,
		3,	3,	1,	0,
		3,	0,	3,	1,
		2,	1,	3,	0,
		1,	3,	2,	3,
		3,	3,	1,	2,
		3,	3,	2,	2,
		3,	1,	0,	1,
		3,	3,	3,	2,
		3,	0,	1,	3,
		3,	3,	0,	3,
		0,	3,	0,	2,
		3,	2,	2,	0,
		3,	0,	0,	2,
		3,	3,	0,	1,
		3,	3,	2,	3,
		1,	0,	3,	3,
		1,	3,	1,	1,
		2,	0,	2,	3,
		3,	3,	1,	0,
		3,	3,	1,	0,
		1,	0,	0,	1,
		1,	0,	2,	1,
		0,	1,	0,	3,
		1,	3,	0,	1,
		1,	0,	0,	2,
		3,	1,	2,	1,
		3,	3,	2,	3,
		3,	3,	1,	3,
		0,	3,	0,	3,
		2,	3,	0,	3,
		3,	3,	1,	0,
		3,	0,	3,	1,
		2,	1,	3,	0,
		1,	3,	2,	3,
		3,	3,	1,	2,
		3,	3,	2,	2,
		3,	1,	0,	1,
		3,	3,	3,	2,
		3,	0,	1,	3,
		3,	3,	0,	3,
		0,	3,	0,	2,
		3,	2,	2,	0,
		3,	0,	0,	2,
		3,	3,	0,	1,
		3,	3,	2,	3,
		1,	0,	3,	3,
		1,	3,	1,	1,
		2,	0,	2,	3,
		3,	3,	1,	0,
		3,	3,	1,	0,
		1,	0,	0,	1,
		1,	0,	2,	1,
		0,	1,	0,	3,
		1,	3,	0,	1,
		1,	0,	0,	2,
		3,	1,	2,	1,
		3,	3,	2,	3,
		3,	3,	1,	3,
		0,	3,	0,	3,
		2,	3,	0,	3,
		3,	3,	1,	0,
		3,	0,	3,	1,
		2,	1,	3,	0,
		1,	3,	2,	3,
		3,	3,	1,	2,
		3,	3,	2,	2,
		3,	1,	0,	1,
		3,	3,	3,	2,
		3,	0,	1,	3,
		3,	3,	0,	3,
		0,	3,	0,	2,
		3,	2,	2,	0,
		3,	0,	0,	2,
		3,	3,	0,	1,
		3,	3,	2,	3,
		1,	0,	3,	3,
		1,	3,	1,	1,
		2,	0,	2,	3,
		3,	3,	1,	0,
		3,	3,	1,	0,
		1,	0,	0,	1,
		1,	0,	2,	1,
		0,	1,	0,	3,
		1,	3,	0,	1,
		1,	0,	0,	2,
		3,	1,	2,	1,
		3,	3,	2,	3,
		3,	3,	1,	3,
		0,	3,	0,	3,
		2,	3,	0,	3,
		3,	3,	1,	0,
		3,	0,	3,	1,
		2,	1,	3,	0,
		1,	3,	2,	3,
		3,	3,	1,	2,
		3,	3,	2,	2,
		3,	1,	0,	1,
		3,	3,	3,	2,
		3,	0,	1,	3,
		3,	3,	0,	3,
		0,	3,	0,	2,
		3,	2,	2,	0,
		3,	0,	0,	2,
		3,	3,	0,	1,
		3,	3,	2,	3,
		1,	0,	3,	3,
		1,	3,	1,	1,
		2,	0,	2,	3,
		3,	3,	1,	0,
		3,	3,	1,	0,
		1,	0,	0,	1,
		1,	0,	2,	1,
		0,	1,	0,	3,
		1,	3,	0,	1,
		1,	0,	0,	2,
		3,	1,	2,	1,
	};

	enum ModelType
	{
		Chimp, // 1
		Bird, // 3
		Crocodile, // 1
		Pony, // 8
	};


	const std::array<ModelInfo, 4>& getModelInfos()
	{
		static const std::array<ModelInfo, 4> modelInfos = [] {
			ModelInfo modelChimp;

			modelChimp.modelPath = "resources/chimp/chimp.FBX";
			modelChimp.texturePath = "resources/chimp/chimp_diffuse.jpg";
			modelChimp.maxAnimationNumber = 1;


			ModelInfo modelBird;

			modelBird.modelPath = "resources/bird/Bird.FBX";
			modelBird.texturePath = "resources/bird/Fogel_Mat_Diffuse_Color.png";
			modelBird.maxAnimationNumber = 3;

			modelBird.scaleX = 0.0025f;
			modelBird.scaleY = 0.0025f;
			modelBird.scaleZ = 0.0025f;

			ModelInfo modelCrocodile;

			modelCrocodile.modelPath = "resources/crocodile/Crocodile.fbx";
			modelCrocodile.texturePath = "resources/crocodile/Crocodile.jpg";

			modelCrocodile.scaleX = 0.015f;
			modelCrocodile.scaleY = 0.015f;
			modelCrocodile.scaleZ = 0.015f;

			modelCrocodile.maxAnimationNumber = 1;


			ModelInfo modelPony;

			modelPony.modelPath = "resources/shetlandponyamber/ShetlandPonyAmberM.fbx";
			modelPony.texturePath = "resources/shetlandponyamber/shetlandponyamber.png";
			modelPony.maxAnimationNumber = 8;

			modelPony.scaleX = 0.015f;
			modelPony.scaleY = 0.015f;
			modelPony.scaleZ = 0.015f;

			return std::array<ModelInfo, 4>{ modelChimp, modelBird, modelCrocodile, modelPony };
		}();

		return modelInfos;
	}

	ModelInfo getModelInfo(ModelType type)
	{
		return getModelInfos()[type];
	}
}

#define M_PI       3.14159265358979323846   // pi

Scene::Scene(IRender& render, RenderGuiData guiData) :
	m_render{ render }
{

	auto modelInfo = getModelInfo(ModelType::Pony);

	for (int j = 0; j < 10; ++j)
	{
		for (int i = 0; i < guiData.modelNumber / 10; ++i)
		{
			auto modelInfo = getModelInfo(ModelType(fixedRundom[(i + 1) * (j + 1)]));

			int yOffset = i / 5;

			modelInfo.posY += yOffset * 4;
			modelInfo.posX += (i - (yOffset * 5)) * (i % 2 == 0 ? 4 : 2);
			modelInfo.posZ -= j * 4;
			modelInfo.animationNumber = ((i + 1) * (j + 1)) % modelInfo.maxAnimationNumber;

			m_modelInfos.push_back(modelInfo);
		}
	}
}


void Scene::run()
{
	m_render.startRenderLoop(m_modelInfos);
}
