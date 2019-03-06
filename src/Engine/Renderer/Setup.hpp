#pragma once

#include <string_view>

namespace Engine
{
	class Scene;

	namespace Renderer
	{
		namespace Setup
		{
			using SceneType = Engine::Scene;

			constexpr std::string_view assetPath = "Data/DRenderer";
		}
	}
}

