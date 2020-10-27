#include "Scene.h"
#include <array>

namespace
{
	enum ModelType
	{
		Chimp,
		Bird,
		Crocodile,
		Pony,
	};


	const std::array<ModelInfo, 4>& getModelInfos()
	{
		static const std::array<ModelInfo, 4> modelInfos = [] {
			ModelInfo modelChimp;

			modelChimp.modelPath = "resources/chimp/chimp.FBX";
			modelChimp.texturePath = "resources/chimp/chimp_diffuse.jpg";


			ModelInfo modelBird;

			modelBird.modelPath = "resources/bird/Bird.FBX";
			modelBird.texturePath = "resources/bird/Fogel_Mat_Diffuse_Color.png";

			modelBird.scaleX = 0.0025f;
			modelBird.scaleY = 0.0025f;
			modelBird.scaleZ = 0.0025f;

			ModelInfo modelCrocodile;

			modelCrocodile.modelPath = "resources/crocodile/Crocodile.fbx";
			modelCrocodile.texturePath = "resources/crocodile/Crocodile.jpg";

			modelCrocodile.scaleX = 0.015f;
			modelCrocodile.scaleY = 0.015f;
			modelCrocodile.scaleZ = 0.015f;


			ModelInfo modelPony;

			modelPony.modelPath = "resources/shetlandponyamber/ShetlandPonyAmberM.fbx";
			modelPony.texturePath = "resources/shetlandponyamber/shetlandponyamber.png";


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


Scene::Scene(IRender& render) :
	m_render{ render }
{
	int i = 0;
	for (auto modelInfo : getModelInfos())
	{
		++i;

		
		modelInfo.posX += 2 * i;
		
		m_modelInfos.push_back(modelInfo);
	}


	for (int i = 0; i < 100; ++i)
	{
		auto modelInfo = getModelInfo(ModelType((i % 4) ));
		modelInfo.posX += 2 * i;
		m_modelInfos.push_back(modelInfo);
	}
}

void Scene::run()
{
	m_render.startRenderLoop(m_modelInfos);
}
