#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/Variant.hpp>

#include <DEngine/Math/Vector.hpp>

#include <vector>
#include <string>
#include <string_view>

namespace DEngine::Gui
{
	class DockArea : public Widget
	{
	public:
		DockArea();
		
		struct Node;
		struct Layer
		{
			// This rect is relative to the DockArea widget's position.
			Rect rect{};
			Std::Box<Node> root;
		};
		std::vector<Layer> layers;

		u32 gizmoSize = 75;
		u32 resizeHandleThickness = 50;
		u32 resizeHandleLength = 75;
		Math::Vec4 resizeHandleColor = { 1.f, 1.f, 1.f, 0.75f };
		Math::Vec4 deleteLayerGizmoColor = { 1.f, 0.f, 0.f, 0.75f };
		Math::Vec4 dockingHighlightColor = { 0.f, 0.5f, 1.f, 0.5f };

		static constexpr auto cursorPointerID = static_cast<u8>(-1);

		struct State_Normal
		{
		};
		struct State_Moving
		{
			bool movingSplitNode;

			u8 pointerId;
			// Pointer offset relative to window origin
			Math::Vec2 pointerOffset;

			struct HoveredWindow
			{
				uSize layerIndex;
				// As the impl::DA_WindowNode type
				void const* windowNode = nullptr;
				Std::Opt<int> gizmoHighlightOpt;
			};
			Std::Opt<HoveredWindow> hoveredWindowOpt;
			Std::Opt<int> backOuterGizmoHighlightOpt;
		};
		struct State_HoldingTab
		{
			// This is a impl::DA_WindowNode type
			void const* windowBeingHeld = nullptr;
			u8 pointerId;
			// Pointer offset relative to tab origin
			Math::Vec2 pointerOffset;
		};
		struct State_ResizingSplitNode
		{
			uSize layerIndex;
			void const* splitNode = nullptr;
			u8 pointerId;
		};
		using StateDataT = Std::Variant<
			State_Normal,
			State_Moving,
			State_HoldingTab,
			State_ResizingSplitNode>;
		StateDataT stateData{};

		void AddWindow(
			std::string_view title,
			Math::Vec4 color,
			Std::Box<Widget>&& widget);

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) override;

		virtual void CursorClick(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void TouchEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchEvent event) override;

		virtual void InputConnectionLost() override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;
	};
}