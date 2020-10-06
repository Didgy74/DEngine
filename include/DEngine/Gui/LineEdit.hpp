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
			UnsignedInteger,
		};
		Type type = Type::Float;


		Math::Vec4 backgroundColor = { 0.25f, 0.25f, 0.25f, 1.f };
		bool selected = false;

		void* pUserData = nullptr;
		std::function<void(LineEdit& widget)> textChangedPfn = nullptr;

		virtual ~LineEdit();

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 charEvent) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;

		virtual void InputConnectionLost(
			Context& ctx) override;

		virtual void CursorClick(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void TouchEvent(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchEvent touch) override;
	};
}