#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/FixedWidthTypes.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/Variant.hpp>
#include <DEngine/Std/Containers/Str.hpp>

#include <DEngine/Math/Vector.hpp>

#include <vector>
#include <string>

namespace DEngine::Gui
{
	namespace impl { struct DA_Node; }

	class DockArea : public Widget
	{
	public:
		u32 gizmoSize = 75;
		u32 resizeHandleThickness = 50;
		u32 resizeHandleLength = 75;
		u32 tabTextMargin = 0;
		struct Colors
		{
			Math::Vec4 resizeHandle = { 1.f, 1.f, 1.f, 0.5f };
			Math::Vec4 deleteLayerGizmo = { 1.f, 0.f, 0.f, 0.75f };
			Math::Vec4 dockingHighlight = { 0.f, 0.5f, 1.f, 0.5f };
		};
		Colors colors = {};

		DockArea();

		void AddWindow(
			Std::Str title,
			Math::Vec4 color,
			Std::Box<Widget>&& widget);

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
		virtual void CursorExit(
			Context& ctx) override;
		virtual bool CursorMove(
			CursorMoveParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded) override;
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;
		virtual void TextInput(
			Context& ctx,
			Std::FrameAlloc& transientAlloc,
			TextInputEvent const& event) override;
		virtual void EndTextInputSession(
			Context& ctx,
			Std::FrameAlloc& transientAlloc,
			EndTextInputSessionEvent const& event) override;



		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual void CharRemoveEvent(
			Context& ctx,
			Std::FrameAlloc& transientAlloc) override;


		struct Impl;
		friend Impl;
	protected:
		struct Layer
		{
			impl::DA_Node* root = nullptr;
			// This rect is relative to the DockArea widgets position.
			// Do no use this directly. Use the GetLayerRect function.
			Rect rect = {};
			Layer() = default;
			Layer(Layer&& other) noexcept : root{ other.root }, rect{ other.rect }
			{
				other.root = nullptr;
				other.rect = {};
			}
			Layer& operator=(Layer&& other) noexcept
			{
				Clear();
				root = other.root;
				other.root = nullptr;
				rect = other.rect;
				other.rect = {};
				return *this;
			}
			void Clear();
			~Layer() { Clear(); }
		};
		std::vector<Layer> layers;

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
				void const* windowNode;
				Std::Opt<int> gizmoHighlightOpt;
			};
			Std::Opt<HoveredWindow> hoveredWindowOpt;
			Std::Opt<int> backOuterGizmoHighlightOpt;
		};
		struct State_HoldingTab
		{
			// This is a impl::DA_WindowNode type
			void const* windowBeingHeld;
			u8 pointerId;
			// Pointer offset relative to tab origin
			Math::Vec2 pointerOffset;
		};
		struct State_ResizingSplit
		{
			bool resizingBack;
			void const* splitNode;
			u8 pointerId;
		};
		using StateDataT = Std::Variant<
			State_Normal,
			State_Moving,
			State_HoldingTab,
			State_ResizingSplit>;
		StateDataT stateData = State_Normal{};
	};
}