#pragma once

#include "DEngine/Gfx/Gfx.hpp"

#include "DEngine/FixedWidthTypes.hpp"

namespace DEngine::Gfx::Vk
{
	bool InitializeBackend(Data& gfxData, InitInfo const& initInfo, void*& buffer);

	void NewViewport(void* apiDataBuffer, uSize& viewportID, void*& imguiTexID);
	void DeleteViewport(void* apiDataBuffer, uSize id);

	void Draw(Data& gfxData, Draw_Params const& drawParams, void* apiDataBuffer);
}