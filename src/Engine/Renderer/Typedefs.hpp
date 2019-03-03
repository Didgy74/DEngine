#pragma once

#include <cstdint>

namespace Engine
{
	namespace Renderer
	{
		namespace Core
		{
			using AssetIntegerType = uint32_t;
		}

		enum class SpriteID : Core::AssetIntegerType {};

		enum class MeshID : Core::AssetIntegerType {};
	}
}
