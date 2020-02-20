#pragma once

#include "DEngine/Gfx/Gfx.hpp"

#include "DEngine/Int.hpp"

namespace DEngine::Gfx::Vk
{
	bool InitializeBackend(Data& gfxData, InitInfo const& initInfo, void*& buffer);

	void NewViewport(void* apiDataBuffer, u8& viewportID, void*& imguiTexID);

	void Draw(Data& gfxData, Draw_Params const& drawParams, void* apiDataBuffer);
}