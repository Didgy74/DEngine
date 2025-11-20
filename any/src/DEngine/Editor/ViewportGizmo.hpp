#pragma once

namespace DEngine::Editor {
	namespace ViewportGizmo {
		enum class GizmoPart {
			ArrowX,
			ArrowY,
			PlaneXY
		};

		struct Arrow {
			f32 capDiameter = 0;
			f32 capLength = 0;
			f32 shaftLength = 0;
			f32 shaftDiameter = 0;
		};

		namespace impl
		{
			[[nodiscard]] constexpr Arrow BuildDefaultArrow() noexcept {
				Arrow arrow = {};
				arrow.capLength = 1 / 3.f;
				arrow.capDiameter = 0.2f;
				arrow.shaftDiameter = 0.1f;
				arrow.shaftLength = 2 / 3.f;
				return arrow;
			}
		}

		constexpr Arrow defaultArrow = impl::BuildDefaultArrow();

		// The gizmo should track the size of the Gui minimum interactable size. There's
		// no obvious part of the gizmo that should be matched to this size, so we've made
		// a formula that makes us scale with it. And then we finely adjust it until it
		// feels right with this multiplier.
		constexpr f32 gizmoScaleMultiplier = 0.5f;

		// Size is relative to the gizmo size
		constexpr f32 defaultPlaneScaleRelative = 0.25f;
		// Relative to the gizmo size
		constexpr f32 defaultPlaneOffsetRelative = 0.25f;

		constexpr f32 defaultRotateCircleInnerRadius = 0.1f;
		constexpr f32 defaultRotateCircleOuterRadius = 1.f - defaultRotateCircleInnerRadius;

		// Converts the window's minimum interactable height into the wanted on-screen size
		// (in pixels) of a unit-length gizmo. Sized so the arrow shaft's on-screen thickness
		// (shaftDiameter of the unit length) matches the physical minimum height, keeping the
		// handle grabbable.
		[[nodiscard]] f32 ComputeTargetSizePx(f32 minimumHeightCm, f32 dpi) noexcept;

		// Returns the world-space scale that makes a unit-length gizmo appear targetSizePx
		// pixels large on screen at its current depth.
		[[nodiscard]] f32 ComputeScale(
			Math::Mat4 const& worldTransform,
			f32 targetSizePx,
			Math::Mat4 const& projection,
			Gui::Extent viewportSize) noexcept;
	}
}
