#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Gfx/Gfx.hpp>

namespace DEngine::Gui
{
	class Image : public Gui::Widget 
	{
	public:
		Gfx::TextureID textureID = Gfx::noTextureID;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;
	};
}