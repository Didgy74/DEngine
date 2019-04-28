#pragma once

#include "DRenderer/Typedefs.hpp"
#include "RendererData.hpp"

#include <vector>
#include <any>
#include <functional>

namespace Engine
{
	namespace Renderer
	{
		namespace OpenGL
		{
			void Initialize(DRenderer::Core::APIDataPointer& apiData, const InitInfo& createInfo);
			void Terminate(void*& apiData);
			void PrepareRenderingEarly(const DRenderer::Core::PrepareRenderingEarlyParams& params);
			void PrepareRenderingLate();
			void Draw();
		}
	}
}

