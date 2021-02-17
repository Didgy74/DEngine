#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Math/Vector.hpp>

#include <string>
#include <string_view>

namespace DEngine::Gui
{
	class Text : public Widget
	{
	public:
		using ParentType = Widget;

		Math::Vec4 color = Math::Vec4::One();
		
		virtual ~Text() override {}

		void String_Set(char const* string);
		void String_PushBack(u8 value);
		void String_PopBack();
		[[nodiscard]] std::string_view StringView() const;

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

	private:
		mutable bool invalidated = true;
		mutable Gui::SizeHint cachedSizeHint{};
		std::string text;
	};
}