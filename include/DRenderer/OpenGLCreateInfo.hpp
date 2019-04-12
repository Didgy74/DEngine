#pragma once

#include "Typedefs.hpp"
#include <functional>

namespace Engine
{
	namespace Renderer
	{
		namespace OpenGL
		{
			struct InitInfo
			{
				std::function<void(void*)> glSwapBuffers;
			};

			bool IsValid(const InitInfo& initInfo, ErrorMessageCallbackPFN callback);
		}
	}
}