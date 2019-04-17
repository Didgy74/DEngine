#pragma once

#include "Typedefs.hpp"
#include <vector>
#include <any>
#include <functional>

namespace Engine
{
	namespace Renderer
	{
		namespace OpenGL
		{
			void Initialize(std::any& apiData, const InitInfo& createInfo);
			void Terminate(std::any& apiData);
			void PrepareRenderingEarly(const std::vector<size_t>& spriteLoadQueue, const std::vector<size_t>& meshLoadQueue);
			void PrepareRenderingLate();
			void Draw();
		}
	}
}

