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

			constexpr bool enableDebugging = true;

			constexpr std::string_view assetPath = "Data";
		}
	}
}

