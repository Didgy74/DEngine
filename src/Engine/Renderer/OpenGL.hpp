#pragma once

#include "Typedefs.hpp"
#include <vector>

namespace Engine
{
	namespace Renderer
	{
		namespace OpenGL
		{
			void Initialize(void*& apiData);
			void Terminate(void*& apiData);
			void PrepareRenderingEarly(const std::vector<SpriteID>& sprites);
			void PrepareRenderingLate();
			void Draw();
		}
	}
}

