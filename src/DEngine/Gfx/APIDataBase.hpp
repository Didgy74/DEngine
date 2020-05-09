#pragma once

#include "DEngine/Gfx/Gfx.hpp"
#include "DEngine/FixedWidthTypes.hpp"

namespace DEngine::Gfx
{
	struct APIDataBase
	{
		virtual ~APIDataBase();

		virtual void Draw(Data& gfxData, DrawParams const& drawParams) = 0;

		virtual void NewViewport(uSize& viewportID, void*& imguiTexID) = 0;
		virtual void DeleteViewport(uSize id) = 0;
	};
}