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
	namespace impl
	{
		struct DA_Node;
		struct DA_SplitNode;
		struct DA_WindowNode;
	}

	class DockArea : public Widget
	{
	public:
		struct Colors
		{
			Math::Vec4 resizeHandle = { 1.f, 1.f, 1.f, 0.5f };
			Math::Vec4 deleteLayerGizmo = { 1.f, 0.f, 0.f, 0.75f };
			Math::Vec4 dockingHighlight = { 0.f, 0.5f, 1.f, 0.5f };
		};
		Colors colors = {};

		static constexpr Extent defaultLayerExtent = { 600, 600 };

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
		virtual bool TouchMove2(
			TouchMoveParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded) override;
		virtual bool TouchPress2(
			TouchPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;
		virtual void TextInput(
			Context& ctx,
			AllocRef const& transientAlloc,
			TextInputEvent const& event) override;
		virtual void EndTextInputSession(
			Context& ctx,
			AllocRef const& transientAlloc,
			EndTextInputSessionEvent const& event) override;

		struct Impl;
		friend Impl;
	protected:


		struct Layer
		{
			// We did not use Std::Box for this because the implementation
			// needs to access the raw pointer, by reference.
			// (Yes, it's using double pointer).
			impl::DA_Node* root = nullptr;
			// This rect is relative to the DockArea widgets position.
			// Do no use this directly. Use the DA_GetLayerRect function.
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

		struct State_Normal {};
		struct State_Moving {
			u8 heldPointerId;
			// Pointer offset relative to window origin
			Math::Vec2 pointerOffset;
			struct HoveredWindow {
				// If windowNode is nullptr, it means
				// we are hovering the back layers outer gizmos.
				// If it is not a nullptr, we are hovering a specific windows
				// inner gizmos.
				impl::DA_WindowNode* windowNode = nullptr;
				[[nodiscard]] impl::DA_WindowNode const* GetWindowNode() const { return windowNode; }
				bool gizmoIsInner = false;
				Std::Opt<int> gizmo;
			};
			Std::Opt<HoveredWindow> hoveredGizmoOpt;
		};
		struct State_HoldingTab {
			// This is a impl::DA_WindowNode type
			impl::DA_WindowNode* windowBeingHeld;
			u8 pointerId;
			uSize tabIndex;
			Math::Vec2 pointerOffsetFromTab;
		};
		struct State_ResizingSplit
		{
			bool resizingBack;
			impl::DA_SplitNode* splitNode;
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