#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Math/Vector.hpp>
#include <DEngine/Std/Containers/Str.hpp>

#include <string>

namespace DEngine::Gui
{
	class Text : public Widget
	{
	public:
		using ParentType = Widget;

		std::string text;
		Math::Vec4 color = Math::Vec4::One();
		f32 relativeScale = 1.f;
		u32 margin = 0;
		bool expandX = true;
		
		virtual ~Text() override {}

		virtual SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;
		virtual void Render2(
			Render_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;

	private:
		struct Impl;
		friend Impl;
	};
}