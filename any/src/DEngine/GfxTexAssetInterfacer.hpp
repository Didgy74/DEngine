#pragma once

#include "DEngine/Gfx/Gfx.hpp"

namespace DEngine::impl {
	class GfxTexAssetInterfacer : public Gfx::TextureAssetInterface {
		virtual char const* get(Gfx::TextureID id) const override
		{
			switch ((int)id){
				case 0:
					//return "data/01.ktx";
				//case 1:
					return "assets/Crate.png";
				case 2:
					return "assets/02.png";
				default:
					return "assets/01.ktx";
			}
		}
	};
}
