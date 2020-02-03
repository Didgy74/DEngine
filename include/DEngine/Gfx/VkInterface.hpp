#pragma once

#include "DEngine/Gfx/Gfx.hpp"

#include "DEngine/Int.hpp"

namespace DEngine::Gfx::Vk
{
	bool InitializeBackend(Data& gfxData, InitInfo const& initInfo, void*& buffer);

	void Draw(Data& gfxData, void* apiDataBuffer, float scale);
}