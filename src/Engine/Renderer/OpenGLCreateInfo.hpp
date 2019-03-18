#pragma once

#include <functional>

namespace Engine
{
	namespace Renderer
	{
		namespace OpenGL
		{
			struct CreateInfo
			{
				std::function<void(void*)> glSwapBuffers;
			};
		}
	}
}