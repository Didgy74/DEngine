#pragma once

#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/MenuBar.hpp>

#include "EditorImpl.hpp"

#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/Std/Utility.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>

#include <iostream>

namespace DEngine::Editor
{
	namespace Gizmo
	{
		struct Test_Ray
		{
			Math::Vec3 origin{};
			Math::Vec3 direction{};
		};

		enum GizmoPart
		{
			ArrowX,
			ArrowY,
			PlaneXY
		};

		struct Arrow
		{
			f32 capSize = 0;
			f32 capLength = 0;
			f32 shaftLength = 0;
			f32 shaftDiameter = 0;
		};

		namespace impl
		{
			[[nodiscard]] constexpr Arrow BuildDefaultArrow() noexcept
			{
				Arrow arrow{};
				arrow.capLength = 1 / 3.f;
				arrow.capSize = 0.2f;
				arrow.shaftDiameter = 0.1f;
				arrow.shaftLength = 2 / 3.f;
				return arrow;
			}
		}

		constexpr Arrow defaultArrow = impl::BuildDefaultArrow();
		constexpr f32 defaultGizmoSizeRelative = 0.33f;
		// Size is relative to the gizmo size
		constexpr f32 defaultPlaneScaleRelative = 0.25f;
		// Relative to the gizmo size
		constexpr f32 defaultPlaneOffsetRelative = 0.25f;

		// screenSpacePos in normalized coordinates
		[[nodiscard]] inline Math::Vec3 DeprojectScreenspacePoint(
			Math::Vec2 screenSpacePos,
			Math::Mat4 projectionMatrix) noexcept
		{
			Math::Vec4 vector = screenSpacePos.AsVec4(1.f, 1.f);
			vector = projectionMatrix.GetInverse().Value() * vector;
			for (auto& item : vector)
				item /= vector.w;
			return Math::Vec3(vector);
		}

		// target_size - pixels
		[[nodiscard]] inline f32 ComputeScale(
			Math::Mat4 const& worldTransform,
			u32 target_size,
			Math::Mat4 const& projection,
			Gui::Extent viewport_size) noexcept
		{
			f32 const pixelSize = 1.f / viewport_size.height;
			Math::Vec4 zVec = { worldTransform.At(3, 0), worldTransform.At(3, 1), worldTransform.At(3, 2), worldTransform.At(3, 3) };
			return target_size * pixelSize * (projection * zVec).w;
		}

		// Returns the distance from the ray-origin to the intersection point.
		// Cannot be negative.
		[[nodiscard]] inline Std::Opt<f32> IntersectRayPlane(
			Test_Ray ray,
			Math::Vec3 planeNormal,
			Math::Vec3 pointOnPlane) noexcept
		{
			DENGINE_DETAIL_ASSERT(Math::Abs(ray.direction.Magnitude() - 1.f) <= 0.00001f);
			DENGINE_DETAIL_ASSERT(Math::Abs(planeNormal.Magnitude() - 1.f) <= 0.00001f);

			f32 d = Math::Vec3::Dot(planeNormal, pointOnPlane);

			// Compute the t value for the directed line ab intersecting the plane
			f32 t = (d - Math::Vec3::Dot(planeNormal, ray.origin)) / Math::Vec3::Dot(planeNormal, ray.direction);
			// If t is above 0, the intersection is in front of the ray, not behind. Calculate the exact point
			if (t >= 0.0f)
				return Std::Opt<f32>{ t };
			else
				return Std::nullOpt;
		}

		// Returns the distance from the ray-origin to the intersection point.
		// Cannot be negative.
		[[nodiscard]] inline Std::Opt<f32> IntersectRayCylinder(
			Test_Ray ray,
			Math::Vec3 cylinderVertA,
			Math::Vec3 cylinderVertB,
			f32 r) noexcept
		{
			// Reference implementation can be found in Real-Time Collision Detection p. 195
			
			DENGINE_DETAIL_ASSERT(Math::Abs(ray.direction.Magnitude() - 1.f) <= 0.0001f);

			// Also referred to as "d"
			Math::Vec3 const cylinderAxis = cylinderVertB - cylinderVertA;
			// Vector from cylinder base to ray start.
			Math::Vec3 const m = ray.origin - cylinderVertA; 

			Std::Opt<f32> returnVal{};

			f32 const md = Math::Vec3::Dot(m, cylinderAxis);
			f32 const nd = Math::Vec3::Dot(ray.direction, cylinderAxis);
			f32 const dd = cylinderAxis.MagnitudeSqrd();

			f32 const nn = ray.direction.MagnitudeSqrd();
			f32 const mn = Math::Vec3::Dot(m, ray.direction);
			f32 const a = dd * nn - Math::Sqrd(nd);
			f32 const k = m.MagnitudeSqrd() - Math::Sqrd(r);
			f32 const c = dd * k - Math::Sqrd(md);

			if (c < 0.f)
			{
				// The ray origin is inside the infinite cylinder.
				// Check if it's also within the endcaps?
				DENGINE_DETAIL_UNREACHABLE();
			}

			// Check if ray runs parallel to cylinder axis.
			if (Math::Abs(a) < 0.0001f) 
			{
				DENGINE_DETAIL_UNREACHABLE();
				// Segment runs parallel to cylinder axis
				if (c > 0.0f)
					return Std::nullOpt; // �a� and thus the segment lie outside cylinder
				// Now known that segment intersects cylinder; figure out how it intersects
				f32 dist = 0.f;
				if (md < 0.0f)
					dist = -mn / nn; // Intersect segment against �p� endcap
				else if (md > dd)
					dist = (nd - mn) / nn; // Intersect segment against �q� endcap
				else
					dist = 0.0f; // �a� lies inside cylinder

				return Std::Opt<f32>{ dist };
			}

			
			// Intersect with the infinite cylinder.
			{
				f32 const b = dd * mn - nd * md;
				f32 const discr = Math::Sqrd(b) - a * c;
				if (discr >= 0.0f)
				{
					// Discriminant is positive, we have an intersection.
					f32 t = (-b - Math::Sqrt(discr)) / a;
					// If t >= 0, it means the intersection is in front of the ray, not behind.
					if (t >= 0.f)
					{
						Std::Opt<f32> cylinderHit{};

						if (md + t * nd < 0.0f) 
						{
							// Intersection outside cylinder on �p� side
							if (nd > 0.f)
							{
								// Segment is not pointing away from endcap
								t = -md / nd;
								// Keep intersection if Dot(S(t) - p, S(t) - p) <= r^2
								if (k + 2 * t * (mn + t * nn) <= 0.f)
									cylinderHit = Std::Opt<f32>{ t };
							}
						}
						else if (md + t * nd > dd) {
							// Intersection outside cylinder on �q� side
							if (nd < 0.f)
							{
								// Segment is not pointing away from endcap
								t = (dd - md) / nd;
								// Keep intersection if Dot(S(t) - q, S(t) - q) <= r^2
								if (k + dd - 2 * md + t * (2 * (mn - nd) + t * nn) <= 0.f)
									cylinderHit = Std::Opt<f32>{ t };
							}
						}
						else
						{
							// Segment intersects cylinder between the endcaps; t is correct
							cylinderHit = Std::Opt<f32>{ t };
						}

						if (cylinderHit.HasValue())
						{
							if (!returnVal.HasValue() || (returnVal.HasValue() && cylinderHit.Value() < returnVal.Value()))
								returnVal = cylinderHit;
						}
					}
				}
			}

			// Intersect endcaps
			Math::Vec3 const cylinderCapVertices[2] = { cylinderVertA, cylinderVertB };
			for (auto const& endcap : cylinderCapVertices)
			{
				Std::Opt<f32> const hit = IntersectRayPlane(
					ray,
					cylinderAxis.Normalized(),
					endcap);
				if (hit.HasValue())
				{
					f32 const distance = hit.Value();
					Math::Vec3 const hitPoint = ray.origin + ray.direction * distance;
					if ((hitPoint - endcap).MagnitudeSqrd() <= Math::Sqrd(r))
					{
						// Hit point is on endcap
						if (!returnVal.HasValue() || (returnVal.HasValue() && distance < returnVal.Value()))
							returnVal = Std::Opt{ distance };
					}
				}
			}
			
			return returnVal;
		}

		// World-transform cannot include scale. Bake it into the arrow-struct.
		[[nodiscard]] inline Std::Opt<f32> IntersectArrow(
			Test_Ray ray, 
			Arrow arrow, 
			Math::Mat4 const& worldTransform) noexcept
		{
			Std::Opt<f32> result;
			Math::Vec3 const vertex1 = Math::Vec3(worldTransform * Math::Vec4{ 0, 0, 0, 1 });
			Math::Vec3 const vertex2 = Math::Vec3(worldTransform * Math::Vec4{ 0, 0, arrow.shaftLength, 1 });
			f32 const radius = 0.5f * arrow.shaftDiameter;

			result = IntersectRayCylinder(
				ray,
				vertex1,
				vertex2,
				radius);

			return result;
		}

		// Translate along a line
		[[nodiscard]] inline Math::Vec3 TranslateAlongPlane(
			Test_Ray ray,
			Math::Vec3 planeNormal,
			Math::Vec3 initialPosition,
			Test_Ray initialRay) noexcept
		{
			DENGINE_DETAIL_ASSERT(Math::Abs(ray.direction.Magnitude() - 1.f) <= 0.00001f);
			DENGINE_DETAIL_ASSERT(Math::Abs(planeNormal.Magnitude() - 1.f) <= 0.00001f);
			DENGINE_DETAIL_ASSERT(Math::Abs(initialRay.direction.Magnitude() - 1.f) <= 0.00001f);

			// Calculate cursor offset that we'll use to prevent the center of the object from snapping to the cursor
			Math::Vec3 offset{};
			Std::Opt<f32> hitA = Gizmo::IntersectRayPlane(initialRay, planeNormal, initialPosition);
			if (hitA.HasValue())
				offset = initialPosition - (initialRay.origin + initialRay.direction * hitA.Value());
			
			Std::Opt<f32> hitB = Gizmo::IntersectRayPlane(ray, planeNormal, initialPosition);
			if (hitB.HasValue())
				return ray.origin + ray.direction * hitB.Value() + offset;
			else
				return initialPosition;
		}
	}

	class InternalViewportWidget : public Gui::Widget
	{
	public:
		Gfx::ViewportID viewportId = Gfx::ViewportID::Invalid;
		Gfx::Context* gfxCtx = nullptr;
		EditorImpl* implData = nullptr;

		mutable bool isVisible = false;

		mutable Gui::Extent currentExtent{};
		mutable Gui::Extent newExtent{};
		mutable bool currentlyResizing = false;
		mutable u32 extentCorrectTickCounter = 0;
		mutable bool extentsAreInitialized = false;

		bool isCurrentlyClicked = false;
		bool isCurrentlyHoldingGizmo = false;
		Std::Opt<u8> gizmo_touchID{};
		Gizmo::GizmoPart gizmo_holdingPart{};
		Gizmo::Test_Ray gizmo_initialRay{};
		Math::Vec3 gizmo_initialPos{};


		int joystickPixelRadius = 50;
		int joystickPixelDeadZone = 10;
		struct JoyStick
		{
			Std::Opt<u8> touchID{};
			bool isClicked = false;
			mutable Math::Vec2Int originPosition{};
			Math::Vec2Int currentPosition{};
		};
		// 0 is left, 1 is right
		JoyStick joysticks[2]{};

		struct Camera
		{
			Math::Vec3 position{ 0.f, 0.f, 1.f };
			Math::UnitQuat rotation = Math::UnitQuat::FromEulerAngles(0, 180.f, 0.f);
			f32 verticalFov = 60.f;
		};
		Camera cam{};

		InternalViewportWidget(EditorImpl& implData, Gfx::Context& gfxCtxIn) :
			gfxCtx(&gfxCtxIn),
			implData(&implData)
		{
			implData.viewportWidgets.push_back(this);

			auto newViewportRef = gfxCtx->NewViewport();
			viewportId = newViewportRef.ViewportID();
		}

		virtual ~InternalViewportWidget() override
		{
			gfxCtx->DeleteViewport(viewportId);
			auto ptrIt = Std::FindIf(
				implData->viewportWidgets.begin(),
				implData->viewportWidgets.end(),
				[this](decltype(implData->viewportWidgets)::value_type const& val) -> bool { return val == this; });
			DENGINE_DETAIL_ASSERT(ptrIt != implData->viewportWidgets.end());
			implData->viewportWidgets.erase(ptrIt);
		}

		[[nodiscard]] static Math::Vec2 GetNormalizedViewportPosition(
			Math::Vec2Int cursorPos,
			Gui::Rect viewportRect) noexcept;

		[[nodiscard]] Math::Mat4 GetViewMatrix() const noexcept;

		[[nodiscard]] Math::Mat4 GetPerspectiveMatrix(f32 aspectRatio) const noexcept;

		Math::Mat4 GetProjectionMatrix(f32 aspectRatio) const noexcept;

		void ApplyCameraRotation(Math::Vec2 input) noexcept;

		void ApplyCameraMovement(Math::Vec3 move, f32 speed) noexcept;

		Math::Vec2 GetLeftJoystickVec() const noexcept;

		Math::Vec2 GetRightJoystickVec() const noexcept;

		void TickTest(f32 deltaTime) noexcept;

		// Pixel space
		void UpdateJoystickOrigin(Gui::Rect widgetRect) const noexcept;

		virtual void CursorClick(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Math::Vec2Int cursorPos,
			Gui::CursorClickEvent event) override;

		virtual void CursorMove(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::CursorMoveEvent event) override;

		virtual void TouchEvent(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::TouchEvent touch) override;

		virtual Gui::SizeHint GetSizeHint(
			Gui::Context const& ctx) const override
		{
			Gui::SizeHint returnVal{};
			returnVal.preferred = { 450, 450 };
			returnVal.expandX = true;
			returnVal.expandY = true;
			return returnVal;
		}

		virtual void Render(
			Gui::Context const& ctx,
			Gui::Extent framebufferExtent,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::DrawInfo& drawInfo) const override;
	};


	class ViewportWidget : public Gui::StackLayout 
	{
	public:
		ViewportWidget(EditorImpl& implData, Gfx::Context& ctx) :
			Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
		{
			// Generate top navigation bar
			Gui::MenuBar* menuBar = new Gui::MenuBar(Gui::MenuBar::Direction::Horizontal);
			AddLayout2(Std::Box<Gui::Layout>{ menuBar });
			
			menuBar->AddSubmenuButton(
				"Camera",
				[](
					Gui::MenuBar& newMenuBar)
				{
					newMenuBar.AddMenuButton(
						"FreeLook",
						[]()
						{

						});
				});

			InternalViewportWidget* viewport = new InternalViewportWidget(implData, ctx);
			AddWidget2(Std::Box<Gui::Widget>{ viewport });
		}
	};
}