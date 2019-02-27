#pragma once

namespace Engine
{
	namespace Renderer
	{
		namespace OpenGL
		{
			void Initialize(void*& apiData);
			void Terminate(void*& apiData);
			void PrepareRendering();
			void Draw();
		}
	}
}

