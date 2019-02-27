#pragma once

namespace Engine
{
	namespace Renderer
	{
		namespace Vulkan
		{
			void Initialize(void*& apiData);
			void Terminate(void*& apiData);
			void PrepareRendering();
			void Draw();
		}
	}
}
