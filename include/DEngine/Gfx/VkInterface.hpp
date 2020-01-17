#pragma once

#include "DEngine/Gfx/Gfx.hpp"

#include "DEngine/Int.hpp"

namespace DEngine::Gfx::Vk
{
	uSize GetAPIDataSize();

	bool InitializeBackend(Data& gfxData, const InitInfo& initInfo, void* buffer);

	void Draw(Data& gfxData, void* apiDataBuffer, float scale);
}