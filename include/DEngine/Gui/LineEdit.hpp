#pragma once

#include <DEngine/Gui/Text.hpp>

#include <functional>

namespace DEngine::Gui
{
	class LineEdit : public Text
	{
	public:

		enum class Type
		{
			Float,
			Integer,
		};
		Type type = Type::Float;


		Math::Vec4 backgroundColor = { 0.25f, 0.25f, 0.25f, 1.f };
		bool selected = false;

		void* pUserData = nullptr;
		std::function<void(LineEdit& widget)> textChangedPfn = nullptr;

		virtual void Render(
			Context& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			DrawInfo& drawInfo) const override;

		virtual void CharEvent(
			Context& ctx,
			u32 charEvent) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;

		virtual void CursorClick(
			Rect widgetRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void TouchEvent(
			Rect widgetRect,
			Gui::TouchEvent touch) override;
	};
}