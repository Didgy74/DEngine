#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Math/Vector.hpp>

#include <string>

namespace DEngine::Gui
{
	class Text : public Widget
	{
	public:
		std::string text;
		Math::Vec4 color = Math::Vec4::One();
		
		virtual ~Text() override {}

		virtual void Render(
			Context& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			DrawInfo& drawInfo) const override;
	};
}