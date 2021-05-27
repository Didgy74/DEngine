#include "ViewportWidget.hpp"

#include <DEngine/Editor/Joystick.hpp>

#include <DEngine/Math/LinearTransform3D.hpp>
#include <DEngine/Std/Trait.hpp>

#include <DEngine/Gui/AnchorArea.hpp>

namespace DEngine::Editor::impl
{
	// Translates cursorpos into [-1, 1] of the viewport-rect.
	[[nodiscard]] Math::Vec2 GetNormalizedViewportCoord(
		Math::Vec2 cursorPos,
		Gui::Rect const& viewportRect) noexcept
	{
		Math::Vec2 cursorNormalizedPos = {
			(cursorPos.x - viewportRect.position.x) / viewportRect.extent.width,
			(cursorPos.y - viewportRect.position.y) / viewportRect.extent.height };
		// Now go from [0, 1] to [-1, 1]
		cursorNormalizedPos -= { 0.5f, 0.5f };
		cursorNormalizedPos *= 2.f;
		cursorNormalizedPos.y = -cursorNormalizedPos.y;
		return cursorNormalizedPos;
	}

	// Returns the distance from the ray-origin to the intersection point.
	// Cannot be negative.
	[[nodiscard]] Std::Opt<f32> IntersectRayPlane(
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDirection,
		Math::Vec3 planeNormal,
		Math::Vec3 pointOnPlane) noexcept
	{
		DENGINE_DETAIL_ASSERT(Math::Abs(rayDirection.Magnitude() - 1.f) <= 0.00001f);
		DENGINE_DETAIL_ASSERT(Math::Abs(planeNormal.Magnitude() - 1.f) <= 0.00001f);

		f32 d = Math::Vec3::Dot(planeNormal, pointOnPlane);

		// Compute the t value for the directed line ab intersecting the plane
		f32 t = (d - Math::Vec3::Dot(planeNormal, rayOrigin)) / Math::Vec3::Dot(planeNormal, rayDirection);
		// If t is above 0, the intersection is in front of the ray, not behind.
		if (t >= 0.0f)
			return Std::Opt<f32>{ t };
		else
			return Std::nullOpt;
	}

	[[nodiscard]] Std::Opt<f32> IntersectRayTri(
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir,
		Math::Vec3 aIn,
		Math::Vec3 bIn,
		Math::Vec3 cIn)
	{
		// Moller-Trumbore Intersection algorithm

		Math::Vec3 rayVector = rayDir;

		f32 const EPSILON = 0.0000001f;
		Math::Vec3 vertex0 = aIn;
		Math::Vec3 vertex1 = bIn;
		Math::Vec3 vertex2 = cIn;
		Math::Vec3 edge1, edge2, h, s, q;
		f32 a, f, u, v;
		edge1 = vertex1 - vertex0;
		edge2 = vertex2 - vertex0;
		//h = rayVector.crossProduct(edge2);
		h = Math::Vec3::Cross(rayVector, edge2);
		//a = edge1.dotProduct(h);
		a = Math::Vec3::Dot(edge1, h);
		if (a > -EPSILON && a < EPSILON)
			return Std::nullOpt;    // This ray is parallel to this triangle.
		f = 1.0f / a;
		s = rayOrigin - vertex0;
		// u = f * s.dotProduct(h);
		u = f * Math::Vec3::Dot(s, h);
		if (u < 0.0 || u > 1.0)
			return Std::nullOpt;
		//q = s.crossProduct(edge1);
		q = Math::Vec3::Cross(s, edge1);
		//v = f * rayVector.dotProduct(q);
		v = f * Math::Vec3::Dot(rayVector, q);
		if (v < 0.0f || u + v > 1.0f)
			return Std::nullOpt;
		// At this stage we can compute t to find out where the intersection point is on the line.
		//float t = f * edge2.dotProduct(q);
		f32 t = f * Math::Vec3::Dot(edge2, q);
		if (t > EPSILON) // ray intersection
		{
			//outIntersectionPoint = rayOrigin + rayVector * t;
			return Std::Opt<f32>{ t };
		}
		else // This means that there is a line intersection but not a ray intersection.
			return Std::nullOpt;
	}

	// Returns the distance from the ray-origin to the intersection point.
	// Cannot be negative.
	[[nodiscard]] Std::Opt<f32> IntersectRayCylinder(
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir,
		Math::Vec3 cylinderVertA,
		Math::Vec3 cylinderVertB,
		f32 cylinderRadius) noexcept
	{
		// Reference implementation can be found in Real-Time Collision Detection p. 195

		DENGINE_DETAIL_ASSERT(Math::Abs(rayDir.Magnitude() - 1.f) <= 0.0001f);

		// Also referred to as "d"
		Math::Vec3 const cylinderAxis = cylinderVertB - cylinderVertA;
		// Vector from cylinder base to ray start.
		Math::Vec3 const m = rayOrigin - cylinderVertA;

		Std::Opt<f32> returnVal{};

		f32 const md = Math::Vec3::Dot(m, cylinderAxis);
		f32 const nd = Math::Vec3::Dot(rayDir, cylinderAxis);
		f32 const dd = cylinderAxis.MagnitudeSqrd();

		f32 const nn = rayDir.MagnitudeSqrd();
		f32 const mn = Math::Vec3::Dot(m, rayDir);
		f32 const a = dd * nn - Math::Sqrd(nd);
		f32 const k = m.MagnitudeSqrd() - Math::Sqrd(cylinderRadius);
		f32 const c = dd * k - Math::Sqrd(md);

		if (c < 0.f)
		{
			// The ray origin is inside the infinite cylinder.
			// Check if it's also within the endcaps?
			DENGINE_IMPL_UNREACHABLE();
		}

		// Check if ray runs parallel to cylinder axis.
		if (Math::Abs(a) < 0.0001f)
		{
			DENGINE_IMPL_UNREACHABLE();
			// Segment runs parallel to cylinder axis
			if (c > 0.0f)
				return Std::nullOpt; // a and thus the segment lie outside cylinder
			// Now known that segment intersects cylinder; figure out how it intersects
			f32 dist = 0.f;
			if (md < 0.0f)
				dist = -mn / nn; // Intersect segment against p endcap
			else if (md > dd)
				dist = (nd - mn) / nn; // Intersect segment against q endcap
			else
				dist = 0.0f; // a lies inside cylinder

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
						// Intersection outside cylinder on p side
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
						// Intersection outside cylinder on q side
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
			auto const hit = IntersectRayPlane(
				rayOrigin,
				rayDir,
				cylinderAxis.GetNormalized(),
				endcap);
			if (hit.HasValue())
			{
				f32 const distance = hit.Value();
				Math::Vec3 const hitPoint = rayOrigin + rayDir * distance;
				if ((hitPoint - endcap).MagnitudeSqrd() <= Math::Sqrd(cylinderRadius))
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
	[[nodiscard]] Std::Opt<f32> IntersectArrow(
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir,
		Gizmo::Arrow arrow,
		Math::Mat4 const& worldTransform) noexcept
	{
		Std::Opt<f32> result;
		Math::Vec3 const vertex1 = (worldTransform * Math::Vec4{ 0, 0, 0, 1 }).AsVec3();
		Math::Vec3 const vertex2 = (worldTransform * Math::Vec4{ 0, 0, arrow.shaftLength, 1 }).AsVec3();
		f32 const radius = 0.5f * arrow.shaftDiameter;

		result = IntersectRayCylinder(
			rayOrigin,
			rayDir,
			vertex1,
			vertex2,
			radius);

		return result;
	}

	// Translate along a line
	[[nodiscard]] Math::Vec3 TranslateAlongPlane(
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir,
		Math::Vec3 planeNormal,
		Math::Vec3 initialPosition,
		Math::Vec3 offset) noexcept
	{
		DENGINE_DETAIL_ASSERT(Math::Abs(rayDir.Magnitude() - 1.f) <= 0.00001f);
		DENGINE_DETAIL_ASSERT(Math::Abs(planeNormal.Magnitude() - 1.f) <= 0.00001f);

		Std::Opt<f32> hitB = IntersectRayPlane(rayOrigin, rayDir, planeNormal, initialPosition - offset);
		if (hitB.HasValue())
			return rayOrigin + rayDir * hitB.Value() + offset;
		else
			return initialPosition;
	}

	struct GizmoHitTest_Translate_ReturnT
	{
		Gizmo::GizmoPart gizmoPart;
		f32 distance;
	};
	[[nodiscard]] static Std::Opt<GizmoHitTest_Translate_ReturnT> GizmoHitTest_Translate(
		Math::Vec3 gizmoPosition,
		f32 gizmoRotation,
		f32 gizmoScale,
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir) noexcept
	{
		// Gizmo cannot include scale in the world transform, so we modify the arrow struct
		// to account for the scaling
		Gizmo::Arrow arrow = Gizmo::defaultArrow;
		arrow.capLength *= gizmoScale;
		arrow.capSize *= gizmoScale;
		arrow.shaftDiameter *= gizmoScale;
		arrow.shaftLength *= gizmoScale;

		Std::Opt<f32> closestDistance;
		Gizmo::GizmoPart gizmoPart = {};
		{
			// Next we handle the X arrow
			Math::Mat4 worldTransform = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Y, Math::pi / 2);
			worldTransform = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, gizmoRotation) * worldTransform;
			Math::LinAlg3D::SetTranslation(worldTransform, gizmoPosition);

			Std::Opt<f32> hit = IntersectArrow(rayOrigin, rayDir, arrow, worldTransform);
			if (hit.HasValue() && (!closestDistance.HasValue() || hit.Value() < closestDistance.Value()))
			{
				closestDistance = hit.Value();
				gizmoPart = Gizmo::GizmoPart::ArrowX;
			}
		}
		{
			// Next we handle the Y arrow
			Math::Mat4 worldTransform = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::X, -Math::pi / 2);
			worldTransform = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, gizmoRotation) * worldTransform;
			Math::LinAlg3D::SetTranslation(worldTransform, gizmoPosition);

			Std::Opt<f32> hit = IntersectArrow(rayOrigin, rayDir, arrow, worldTransform);
			if (hit.HasValue() && (!closestDistance.HasValue() || hit.Value() < closestDistance.Value()))
			{
				closestDistance = hit.Value();
				gizmoPart = Gizmo::GizmoPart::ArrowY;
			}
		}

		// Then we check the XY quad
		{
			Std::Opt<f32> hit = IntersectRayPlane(rayOrigin, rayDir, Math::Vec3::Forward(), gizmoPosition);
			if (hit.HasValue())
			{
				f32 planeScale = gizmoScale * Gizmo::defaultPlaneScaleRelative;
				f32 planeOffset = gizmoScale * Gizmo::defaultPlaneOffsetRelative;

				Math::Vec3 quadPosition = gizmoPosition + Math::Vec2{ 1.f, 1.f }.GetRotated(gizmoRotation).AsVec3() * planeOffset;

				Math::Vec3 point = rayOrigin + rayDir * hit.Value();
				Math::Vec3 pointRelative = point - quadPosition;
				Math::Vec2 up = Math::Vec2::Up().GetRotated(gizmoRotation);
				f32 dotA = Math::Vec3::Dot(pointRelative, Math::Vec2::Right().GetRotated(gizmoRotation).AsVec3());
				f32 dotB = Math::Vec3::Dot(pointRelative, Math::Vec2::Up().GetRotated(gizmoRotation).AsVec3());
				if (dotA >= -planeScale / 2.f && dotA <= planeScale / 2.f && dotB >= -planeScale / 2.f && dotB <= planeScale / 2.f)
				{
					if (hit.HasValue() && (!closestDistance.HasValue() || hit.Value() < closestDistance.Value()))
					{
						closestDistance = hit.Value();
						gizmoPart = Gizmo::GizmoPart::PlaneXY;
					}
				}
			}
		}

		if (closestDistance.HasValue())
		{
			GizmoHitTest_Translate_ReturnT returnVal = {};
			returnVal.distance = closestDistance.Value();
			returnVal.gizmoPart = gizmoPart;
			return returnVal;
		}
		else
			return Std::nullOpt;
	}

	// Returns distance to hitpoint if any
	[[nodiscard]] static Std::Opt<f32> GizmoHitTest_Rotate(
		Math::Vec3 gizmoPosition,
		f32 scale,
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDirection) noexcept
	{
		auto const hitPointDistOpt = IntersectRayPlane(
			rayOrigin,
			rayDirection,
			Math::Vec3::Forward(),
			gizmoPosition);

		if (!hitPointDistOpt.HasValue())
			return {};

		f32 const distance = hitPointDistOpt.Value();
		Math::Vec3 hitPoint = rayOrigin + (rayDirection * distance);

		f32 const relativeDistSqrd = (hitPoint - gizmoPosition).MagnitudeSqrd();
		// Check if distance between transform position and hitpoint is within border
		constexpr f32 outerCircleRadius = Gizmo::defaultRotateCircleOuterRadius;
		constexpr f32 innerCircleRadius = Gizmo::defaultRotateCircleInnerRadius;
		f32 const innerDistance = (outerCircleRadius - innerCircleRadius) * scale;
		f32 const outerDistance = (outerCircleRadius + innerCircleRadius) * scale;

		if (relativeDistSqrd >= Math::Sqrd(innerDistance) && relativeDistSqrd <= Math::Sqrd(outerDistance))
			return hitPointDistOpt;
		else
			return {};
	}

	struct GizmoHitTestReturn_T
	{
		Gizmo::GizmoPart part;
		bool hitRotateGizmo;
		f32 rotateOffset;
		Math::Vec3 normalizedOffset;
	};

	[[nodiscard]] static Std::Opt<GizmoHitTestReturn_T> GizmoHitTest(
		InternalViewportWidget const& widget,
		Gui::Rect const& widgetRect,
		Math::Vec2 pointerPos,
		Math::Vec3 gizmoPosition,
		f32 gizmoRotation,
		GizmoType gizmoType)
	{
		Math::Mat4 worldTransform = Math::Mat4::Identity();
		Math::LinAlg3D::SetTranslation(worldTransform, gizmoPosition);

		f32 scale = Gizmo::ComputeScale(
			worldTransform,
			widget.BuildProjectionMatrix(widgetRect.extent.Aspect()),
			widgetRect.extent);

		Math::Vec3 rayOrigin = widget.cam.position;
		Math::Vec3 rayDir = widget.BuildRayDirection(widgetRect, pointerPos);

		if (gizmoType == GizmoType::Translate)
		{
			auto const hitOpt = GizmoHitTest_Translate(
				gizmoPosition,
				gizmoRotation,
				scale,
				rayOrigin,
				rayDir);
			if (hitOpt.HasValue())
			{
				auto const& hit = hitOpt.Value();
				GizmoHitTestReturn_T returnVal = {};
				returnVal.part = hit.gizmoPart;
				returnVal.normalizedOffset = (gizmoPosition - (rayOrigin + rayDir * hit.distance)) * (1.f / scale);
				return returnVal;
			}
		}
		else if (gizmoType == GizmoType::Rotate)
		{
			auto const hitDistanceOpt = GizmoHitTest_Rotate(
				gizmoPosition,
				scale,
				rayOrigin,
				rayDir);
			if (hitDistanceOpt.HasValue())
			{
				GizmoHitTestReturn_T returnVal = {};
				returnVal.hitRotateGizmo = true;
				// Calculate rotation offset from the hitpoint.
				Math::Vec3 hitPoint = rayOrigin + rayDir * hitDistanceOpt.Value();
				Math::Vec2 hitPointRel = (hitPoint - gizmoPosition).AsVec2().GetNormalized();
				Math::Vec2 transformUp = Math::Vec2::Up().GetRotated(gizmoRotation);
				returnVal.rotateOffset = -Math::Vec2::SignedAngle(hitPointRel, transformUp);
				return returnVal;
			}
		}

		return Std::nullOpt;
	}

	// Returns the distance of the hit.
	[[nodiscard]] static Std::Opt<f32> Intersect_Ray_PhysicsCollider(
		InternalViewportWidget& widget,
		Std::Span<Math::Vec2 const> vertices,
		Math::Vec2 position,
		f32 rotation,
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir)
	{
		Math::Mat4 transMat = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, rotation);
		Math::LinAlg3D::SetTranslation(transMat, position.AsVec3());

		// Compare against all trianges to check if we hit it
		for (uSize i = 1; i < vertices.Size() - 1; i += 1)
		{
			Math::Vec2 tri[3] = { vertices[0], vertices[i], vertices[i + 1] };
			for (uSize j = 0; j < 3; j += 1)
				tri[j] = (transMat * tri[j].AsVec4(0.f, 1.f)).AsVec2();

			Std::Opt<f32> distance = impl::IntersectRayTri(
				rayOrigin,
				rayDir,
				tri[0].AsVec3(),
				tri[1].AsVec3(),
				tri[2].AsVec3());
			if (distance.HasValue())
				return distance;
		}

		return Std::nullOpt;
	}

	static void TranslateAlongGizmoAxis(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		Math::Vec2 pointerPos,
		Transform& transform)
	{
		DENGINE_DETAIL_ASSERT(widget.holdingGizmoData.HasValue());

		auto const& gizmoHoldingData = widget.holdingGizmoData.Value();

		Math::Vec3 rayOrigin = widget.cam.position;
		Math::Vec3 rayDir = widget.BuildRayDirection(widgetRect, pointerPos);
		
		auto transformMat = Math::LinAlg3D::Translate(transform.position);
		f32 const scale = Gizmo::ComputeScale(
			transformMat,
			widget.BuildProjectionMatrix(widgetRect.extent.Aspect()),
			widgetRect.extent);

		Math::Vec3 newPos = TranslateAlongPlane(
			rayOrigin,
			rayDir,
			Math::Vec3::Forward(),
			gizmoHoldingData.initialPos,
			gizmoHoldingData.normalizedOffset * scale);
		if (gizmoHoldingData.holdingPart == Gizmo::GizmoPart::ArrowX || gizmoHoldingData.holdingPart == Gizmo::GizmoPart::ArrowY)
		{
			Math::Vec3 movementAxis = {};
			switch (gizmoHoldingData.holdingPart)
			{
				case Gizmo::GizmoPart::ArrowX:
					movementAxis = Math::Vec2::Right().GetRotated(transform.rotation).AsVec3();
					break;
				case Gizmo::GizmoPart::ArrowY:
					movementAxis = Math::Vec2::Up().GetRotated(transform.rotation).AsVec3();
					break;
				default:
					DENGINE_IMPL_UNREACHABLE();
					break;
			}
			newPos = gizmoHoldingData.initialPos + movementAxis * Math::Vec3::Dot(movementAxis, newPos - gizmoHoldingData.initialPos);
		}
		else
		{

		}

		transform.position = newPos;
	}

	static void Gizmo_HandleRotation(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		Math::Vec2 pointerPos,
		f32 rotationOffset,
		Transform& transform)
	{
		Math::Vec3 rayOrigin = widget.cam.position;
		Math::Vec3 rayDir = widget.BuildRayDirection(widgetRect, pointerPos);

		// Check if we hit the plane of the gizmo
		auto distanceOpt = IntersectRayPlane(
			rayOrigin,
			rayDir,
			Math::Vec3::Forward(),
			transform.position);

		if (!distanceOpt.HasValue())
			return;

		Math::Vec3 hitPoint = rayOrigin + rayDir * distanceOpt.Value();
		Math::Vec3 hitPointRel = hitPoint - transform.position;

		// We should technically project the hitpoint as 2D vector onto the plane,
		// But the plane is facing directly towards Z anyways so we just ditch the Z component.

		Math::Vec2 temp = hitPointRel.AsVec2().GetNormalized();
		// Calculate angle between this and the world space Up vector.
		transform.rotation = Math::Vec2::SignedAngle(temp, Math::Vec2::Up()) + rotationOffset;
	}

	static constexpr u8 cursorPointerId = static_cast<u8>(-1);

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(Gui::CursorButton in) noexcept
	{
		switch (in)
		{
			case Gui::CursorButton::Primary: return PointerType::Primary;
			case Gui::CursorButton::Secondary: return PointerType::Secondary;
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}

	struct PointerPress_Pointer
	{
		Math::Vec2 pos;
		u8 id;
		bool pressed;
		PointerType type;
	};
	
	[[nodiscard]] static bool PointerPress(
		InternalViewportWidget& widget,
		Gui::Context& ctx,
		Gui::WindowID windowId,
		Gui::Rect const& widgetRect,
		Gui::Rect const& visibleRect,
		PointerPress_Pointer const& pointer);

	struct PointerMove_Pointer
	{
		Math::Vec2 pos;
		u8 id;
		bool occluded;
	};
	[[nodiscard]] static bool PointerMove(
		InternalViewportWidget& widget,
		Gui::Context& ctx,
		Gui::WindowID windowId,
		Gui::Rect const& widgetRect,
		Gui::Rect const& visibleRect,
		PointerMove_Pointer const& pointer,
		Gui::CursorMoveEvent const* cursorMove)
	{
		bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);

		if (widget.state == InternalViewportWidget::BehaviorState::Gizmo)
		{
			DENGINE_DETAIL_ASSERT(widget.holdingGizmoData.HasValue());
			Entity entity = {};
			Transform* transformPtr = nullptr;
			auto const& gizmoHoldingData = widget.holdingGizmoData.Value();
			if (gizmoHoldingData.pointerId == pointer.id && widget.editorImpl->GetSelectedEntity().HasValue())
			{
				entity = widget.editorImpl->GetSelectedEntity().Value();
				// First check if we have a transform
				Scene& scene = widget.editorImpl->GetActiveScene();
				transformPtr = scene.GetComponent<Transform>(entity);
			}
			if (transformPtr != nullptr)
			{
				Transform& transform = *transformPtr;

				GizmoType const currGizmo = widget.editorImpl->GetCurrentGizmoType();
				if (currGizmo == GizmoType::Translate)
				{
					impl::TranslateAlongGizmoAxis(
						widget,
						widgetRect,
						pointer.pos,
						transform);
				}
				else if (currGizmo == GizmoType::Rotate)
				{
					impl::Gizmo_HandleRotation(
						widget,
						widgetRect,
						pointer.pos,
						gizmoHoldingData.rotationOffset,
						transform);
				}
			}
		}

		if (widget.state == InternalViewportWidget::BehaviorState::FreeLooking &&
			pointer.id == cursorPointerId)
		{
			DENGINE_IMPL_GUI_ASSERT(cursorMove);
			f32 sensitivity = 0.2f;
			Math::Vec2 amount = { (f32)cursorMove->positionDelta.x, (f32)-cursorMove->positionDelta.y };
			widget.ApplyCameraRotation(amount * sensitivity * Math::degToRad);
		}

		return pointerInside;
	}
}

using namespace DEngine;
using namespace DEngine::Editor;

static bool impl::PointerPress(
	InternalViewportWidget& widget,
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect const& widgetRect,
	Gui::Rect const& visibleRect,
	PointerPress_Pointer const& pointer)
{
	bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
	if (!pointerInside && pointer.pressed)
		return false;

	if (widget.holdingGizmoData.HasValue() && 
		widget.holdingGizmoData.Value().pointerId == pointer.id && 
		!pointer.pressed)
	{
		widget.holdingGizmoData = Std::nullOpt;
		widget.state = InternalViewportWidget::BehaviorState::Normal;
		return false;
	}

	// Initiate free-look
	if (pointerInside && 
		pointer.pressed && 
		widget.state == InternalViewportWidget::BehaviorState::Normal &&
		pointer.type == PointerType::Secondary)
	{
		widget.state = InternalViewportWidget::BehaviorState::FreeLooking;
		App::LockCursor(true);
		return true;
	}

	// Stop free-look
	if (!pointer.pressed && 
		widget.state == InternalViewportWidget::BehaviorState::FreeLooking &&
		pointer.type == PointerType::Secondary)
	{
		widget.state = InternalViewportWidget::BehaviorState::Normal;
		App::LockCursor(false);
		return true;
	}

	DENGINE_DETAIL_ASSERT(widget.editorImpl);
	auto& editorImpl = *widget.editorImpl;
	Scene const& scene = editorImpl.GetActiveScene();

	// We don't want to do gizmo stuff if it was the secondary cursor button
	bool doGizmoStuff =
		pointerInside &&
		pointer.pressed &&
		widget.state == InternalViewportWidget::BehaviorState::Normal &&
		pointer.type == PointerType::Primary;
	if (doGizmoStuff)
	{
		bool hitGizmo = false;
		// If we have selected an entity, and it has a Transform component,
		// We want to see if we hit it's gizmo.
		Entity ent = Entity::Invalid;
		Transform const* transformPtr = nullptr;
		if (widget.editorImpl->GetSelectedEntity().HasValue())
		{
			ent = widget.editorImpl->GetSelectedEntity().Value();
			transformPtr = scene.GetComponent<Transform>(ent);
		}
		if (transformPtr)
		{
			Transform const& transform = *transformPtr;
			auto const& hitTestOpt = impl::GizmoHitTest(
				widget,
				widgetRect,
				pointer.pos,
				transform.position,
				transform.rotation,
				editorImpl.GetCurrentGizmoType());
			if (hitTestOpt.HasValue())
			{
				auto const& hit = hitTestOpt.Value();
				InternalViewportWidget::HoldingGizmoData holdingGizmoData = {};
				holdingGizmoData.holdingPart = hit.part;
				holdingGizmoData.initialPos = transform.position;
				holdingGizmoData.normalizedOffset = hit.normalizedOffset;
				holdingGizmoData.rotationOffset = hit.rotateOffset;
				holdingGizmoData.pointerId = pointer.id;
				widget.holdingGizmoData = holdingGizmoData;
				widget.state = InternalViewportWidget::BehaviorState::Gizmo;
				hitGizmo = true;
				return pointerInside;
			}
		}
		// If we didn't hit a gizmo, we want to see if we hit any selectable colliders.
		if (!hitGizmo)
		{
			Std::Opt<Std::Pair<f32, Entity>> hitEntity;

			Math::Vec3 rayOrigin = widget.cam.position;
			Math::Vec3 rayDir = widget.BuildRayDirection(widgetRect, pointer.pos);
			// Iterate over all physics components that also have a transform component
			for (auto const& [entity, rb] : scene.GetAllComponents<Physics::Rigidbody2D>())
			{
				Transform const* transform = widget.editorImpl->scene->GetComponent<Transform>(entity);
				if (transform)
				{
					Math::Vec2 vertices[4] = {
						{-0.5f, 0.5f },
						{ 0.5f, 0.5f },
						{ 0.5f, -0.5f },
						{ -0.5f, -0.5f } };
					Std::Opt<f32> distanceOpt = Intersect_Ray_PhysicsCollider(
						widget,
						{ vertices, 4 },
						transform->position.AsVec2(),
						transform->rotation,
						rayOrigin,
						rayDir);
					if (distanceOpt.HasValue())
					{
						f32 newDist = distanceOpt.Value();
						if (!hitEntity.HasValue() || newDist <= hitEntity.Value().a)
							hitEntity = { newDist, entity };
					}
				}
			}
			if (hitEntity.HasValue())
			{
				widget.editorImpl->SelectEntity(hitEntity.Value().b);
				return pointerInside;
			}
		}
	}

	return pointerInside;
}

// Target_size in pixels
[[nodiscard]] f32 Gizmo::ComputeScale(
	Math::Mat4 const& worldTransform,
	Math::Mat4 const& projection,
	Gui::Extent viewportSize) noexcept
{
	u32 smallestViewportExtent = Math::Min(viewportSize.width, viewportSize.height);
	u32 targetSizePx = smallestViewportExtent * Gizmo::defaultGizmoSizeRelative;
	f32 const pixelSize = 1.f / viewportSize.height;
	Math::Vec4 zVec = { worldTransform.At(3, 0), worldTransform.At(3, 1), worldTransform.At(3, 2), worldTransform.At(3, 3) };
	return targetSizePx * pixelSize * (projection * zVec).w;
}

InternalViewportWidget::InternalViewportWidget(
	EditorImpl& implData, 
	Gfx::Context& gfxCtxIn) 
	: gfxCtx(&gfxCtxIn), editorImpl(&implData)
{
	auto newViewportRef = gfxCtx->NewViewport();
	viewportId = newViewportRef.ViewportID();
}

InternalViewportWidget::~InternalViewportWidget()
{
	gfxCtx->DeleteViewport(viewportId);
}

Math::Mat4 InternalViewportWidget::BuildViewMatrix() const noexcept
{
	Math::Mat4 camMat = Math::LinAlg3D::Rotate_Homo(cam.rotation);
	Math::LinAlg3D::SetTranslation(camMat, cam.position);
	return camMat;
}

Math::Mat4 InternalViewportWidget::BuildPerspectiveMatrix(f32 aspectRatio) const noexcept
{
	return Math::LinAlg3D::Perspective_RH_ZO(
		cam.verticalFov * Math::degToRad, 
		aspectRatio, 
		0.1f,
		100.f);
}

Math::Mat4 InternalViewportWidget::BuildProjectionMatrix(f32 aspectRatio) const noexcept
{
	Math::Mat4 camMat = Math::Mat4::Identity();
	camMat.At(0, 0) = -1;
	//test.At(1, 1) = -1;
	camMat.At(2, 2) = -1;
	camMat = BuildViewMatrix() * camMat;
	camMat = camMat.GetInverse().Value();
	return BuildPerspectiveMatrix(aspectRatio) * camMat;
}

Math::Vec3 InternalViewportWidget::BuildRayDirection(
	Gui::Rect widgetRect, 
	Math::Vec2 pointerPos) const noexcept
{
	auto pointerNormPos = impl::GetNormalizedViewportCoord(pointerPos, widgetRect);

	auto perspMat = BuildPerspectiveMatrix(widgetRect.extent.Aspect());

	// This is our coordinates in Normalized Device Coordinates.
	// We use the inverse perspective matrix to transform this into a real-world value,
	// but with no camera transform applied.
	auto normalizedCoordPos = pointerNormPos.AsVec4(1.f, 1.f);
	auto perspMatInvOpt = perspMat.GetInverse();
	DENGINE_DETAIL_ASSERT(perspMatInvOpt.HasValue());
	auto const& perspMatInv = perspMatInvOpt.Value();
	auto vector = perspMatInv * normalizedCoordPos;
	for (auto& item : vector)
		item /= vector.w;
	vector.x *= -1.f;
	vector.z *= -1.f;
	auto vector2 = Math::LinAlg3D::Rotate(vector.AsVec3(), cam.rotation);
	vector2 = vector2.GetNormalized();
	return vector2;
}

void InternalViewportWidget::ApplyCameraRotation(Math::Vec2 input) noexcept
{
	cam.rotation = Math::UnitQuat::FromVector(Math::Vec3::Up(), -input.x) * cam.rotation;
	// Limit rotation up and down
	auto forward = Math::LinAlg3D::ForwardVector(cam.rotation);
	f32 dot = Math::Vec3::Dot(forward, Math::Vec3::Up());
	constexpr f32 upDownDotProductLimit = 0.9f;
	if ((dot <= -upDownDotProductLimit && input.y < 0) || (dot >= upDownDotProductLimit && input.y > 0))
		input.y = 0;
	cam.rotation = Math::UnitQuat::FromVector(Math::LinAlg3D::RightVector(cam.rotation), -input.y) * cam.rotation;
}

void InternalViewportWidget::ApplyCameraMovement(Math::Vec3 move, f32 speed) noexcept
{
	if (move.MagnitudeSqrd() > 0.f)
	{
		if (move.MagnitudeSqrd() > 1.f)
			move.Normalize();
		Math::Vec3 moveVector{};
		moveVector += Math::LinAlg3D::ForwardVector(cam.rotation) * move.z;
		moveVector += Math::LinAlg3D::RightVector(cam.rotation) * -move.x;
		moveVector += Math::Vec3::Up() * move.y;

		if (moveVector.MagnitudeSqrd() > 1.f)
			moveVector.Normalize();
		cam.position += moveVector * speed;
	}
}

void InternalViewportWidget::Tick() noexcept
{
	if (!currentlyResizing && currentExtent != newExtent)
	{
		extentCorrectTickCounter += 1;
		if (extentCorrectTickCounter >= 30)
			currentExtent = newExtent;
	}
	if (currentlyResizing)
		extentCorrectTickCounter = 0;
}

namespace DEngine::Editor::impl
{
	[[nodiscard]] static Gfx::ViewportUpdate::GizmoType ToGfxGizmoType(Editor::GizmoType in) noexcept
	{
		switch (in)
		{
			case Editor::GizmoType::Translate:
				return Gfx::ViewportUpdate::GizmoType::Translate;
			case Editor::GizmoType::Rotate:
				return Gfx::ViewportUpdate::GizmoType::Rotate;
			case Editor::GizmoType::Scale:
				return Gfx::ViewportUpdate::GizmoType::Scale;
			default:
				break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}
}

Gfx::ViewportUpdate InternalViewportWidget::GetViewportUpdate(
	Context const& editor,
	std::vector<Math::Vec3>& lineVertices,
	std::vector<Gfx::LineDrawCmd>& lineDrawCmds) const noexcept
{
	EditorImpl const& editorImpl = editor.ImplData();
	Scene const& scene = editorImpl.GetActiveScene();

	Gfx::ViewportUpdate returnVal = {};
	returnVal.id = viewportId;
	returnVal.width = currentExtent.width;
	returnVal.height = currentExtent.height;

	f32 aspectRatio = (f32)newExtent.width / (f32)newExtent.height;
	Math::Mat4 projMat = BuildProjectionMatrix(aspectRatio);

	returnVal.transform = projMat;

	if (editorImpl.GetSelectedEntity().HasValue())
	{
		Entity selected = editorImpl.GetSelectedEntity().Value();
		// Find Transform component of this entity

		Transform const* transformPtr = scene.GetComponent<Transform>(selected);

		// Draw gizmo
		if (transformPtr != nullptr)
		{
			Transform const& transform = *transformPtr;

			returnVal.gizmoOpt = Gfx::ViewportUpdate::Gizmo{};
			Gfx::ViewportUpdate::Gizmo& gizmo = returnVal.gizmoOpt.Value();
			gizmo.type = impl::ToGfxGizmoType(editorImpl.GetCurrentGizmoType());
			gizmo.position = transform.position;
			gizmo.rotation = transform.rotation;

			Math::Mat4 worldTransform = Math::Mat4::Identity();
			Math::LinAlg3D::SetTranslation(worldTransform, { transform.position.x, transform.position.y, 0.f });
			f32 scale = Gizmo::ComputeScale(
				worldTransform,
				projMat,
				newExtent);
			gizmo.scale = scale;

			gizmo.quadScale = gizmo.scale * Gizmo::defaultPlaneScaleRelative;
			gizmo.quadOffset = gizmo.scale * Gizmo::defaultPlaneOffsetRelative;
		}

		// Draw debug lines for collider if there is one
		Physics::Rigidbody2D const* rbPtr = scene.GetComponent<Physics::Rigidbody2D>(selected);
		if (rbPtr && transformPtr)
		{
			Transform const& transform = *transformPtr;

			Math::Vec2 vertices[4] = {
				{-0.5f, 0.5f },
				{ 0.5f, 0.5f },
				{ 0.5f, -0.5f },
				{ -0.5f, -0.5f } };

			// We need to build a transform from the physics-body, not the Transform component?
			Math::Mat4 worldTransform = Math::Mat4::Identity();
			worldTransform = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, transform.rotation) * worldTransform;
			Math::LinAlg3D::SetTranslation(worldTransform, transform.position);

			for (uSize i = 0; i < 5; i++)
			{
				Math::Vec2 vertex = vertices[i % 4];
				lineVertices.push_back((worldTransform * vertex.AsVec4(0.f, 1.f)).AsVec3());
			}
			Gfx::LineDrawCmd lineDrawCmd{};
			lineDrawCmd.color = { 0.25f, 0.75f, 0.25f, 1.f };
			lineDrawCmd.vertCount = 5;
			lineDrawCmds.push_back(lineDrawCmd);
		}
	}

	return returnVal;
}

bool InternalViewportWidget::CursorPress(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Math::Vec2Int cursorPos,
	Gui::CursorClickEvent event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.clicked;
	pointer.type = impl::ToPointerType(event.button);
	return impl::PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}

bool InternalViewportWidget::CursorMove(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Gui::CursorMoveEvent event,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.occluded = occluded;
	pointer.pos = { (f32)event.position.x, (f32)event.position.y };
	return impl::PointerMove(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer,
		&event);
}

bool InternalViewportWidget::TouchPressEvent(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Gui::TouchPressEvent event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = event.id;
	pointer.pos = event.position;
	pointer.pressed = event.pressed;
	pointer.type = impl::PointerType::Primary;
	return impl::PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}

bool InternalViewportWidget::TouchMoveEvent(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = event.id;
	pointer.occluded = occluded;
	pointer.pos = event.position;
	return impl::PointerMove(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer,
		nullptr);
}

Gui::SizeHint InternalViewportWidget::GetSizeHint(Gui::Context const& ctx) const
{
	Gui::SizeHint returnVal = {};
	returnVal.preferred = { 450, 450 };
	returnVal.expandX = true;
	returnVal.expandY = true;
	return returnVal;
}

void InternalViewportWidget::Render(
	Gui::Context const& ctx,
	Gui::Extent framebufferExtent,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Gui::DrawInfo& drawInfo) const
{
	isVisible = true;
	currentlyResizing = newExtent != widgetRect.extent;
	newExtent = widgetRect.extent;

	// First draw the viewport.
	Gfx::GuiDrawCmd drawCmd = {};
	drawCmd.type = Gfx::GuiDrawCmd::Type::Viewport;
	drawCmd.viewport.id = viewportId;
	drawCmd.rectPosition.x = (f32)widgetRect.position.x / framebufferExtent.width;
	drawCmd.rectPosition.y = (f32)widgetRect.position.y / framebufferExtent.height;
	drawCmd.rectExtent.x = (f32)widgetRect.extent.width / framebufferExtent.width;
	drawCmd.rectExtent.y = (f32)widgetRect.extent.height / framebufferExtent.height;
	drawInfo.drawCmds->push_back(drawCmd);
}

ViewportWidget::ViewportWidget(EditorImpl& implData, Gfx::Context& ctx) :
	Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
{
	implData.viewportWidgets.push_back(this);

	// Generate top navigation bar
	Gui::Button* settingsBtn = new Gui::Button;
	settingsBtn->text = "Settings";
	AddWidget(Std::Box{ settingsBtn });

	Gui::AnchorArea* anchorArea = new Gui::AnchorArea;
	AddWidget(Std::Box{ anchorArea });

	Joystick* leftJoystick = new Joystick;
	this->leftJoystick = leftJoystick;
	Gui::AnchorArea::Node leftJoystickNode = {};
	leftJoystickNode.anchorX = Gui::AnchorArea::AnchorX::Left;
	leftJoystickNode.anchorY = Gui::AnchorArea::AnchorY::Bottom;
	leftJoystickNode.extent = { 200, 200 };
	leftJoystickNode.widget = Std::Box{ leftJoystick };
	anchorArea->nodes.push_back(Std::Move(leftJoystickNode));

	Joystick* rightJoystick = new Joystick;
	this->rightJoystick = rightJoystick;
	Gui::AnchorArea::Node rightJoystickNode = {};
	rightJoystickNode.anchorX = Gui::AnchorArea::AnchorX::Right;
	rightJoystickNode.anchorY = Gui::AnchorArea::AnchorY::Bottom;
	rightJoystickNode.extent = { 200, 200 };
	rightJoystickNode.widget = Std::Box{ rightJoystick };
	anchorArea->nodes.push_back(Std::Move(rightJoystickNode));

	// Background
	InternalViewportWidget* viewport = new InternalViewportWidget(implData, ctx);
	this->viewport = viewport;
	Gui::AnchorArea::Node background = {};
	background.widget = Std::Box{ viewport };
	anchorArea->nodes.push_back(Std::Move(background));
}

Editor::ViewportWidget::~ViewportWidget()
{
	auto ptrIt = Std::FindIf(
		editorImpl->viewportWidgets.begin(),
		editorImpl->viewportWidgets.end(),
		[this](auto const& val) -> bool { return val == this; });
	DENGINE_DETAIL_ASSERT(ptrIt != editorImpl->viewportWidgets.end());
	editorImpl->viewportWidgets.erase(ptrIt);
}

void Editor::ViewportWidget::Tick(float deltaTime) noexcept
{
	viewport->Tick();

	auto leftVector = leftJoystick->GetVector();
	auto rightVector = rightJoystick->GetVector();

	Math::Vec3 moveVector = {};

	moveVector += Math::LinAlg3D::UpVector(viewport->cam.rotation) * -leftVector.y;
	moveVector += Math::LinAlg3D::RightVector(viewport->cam.rotation) * -leftVector.x;
	moveVector += Math::LinAlg3D::ForwardVector(viewport->cam.rotation) * -rightVector.y;

	if (moveVector.MagnitudeSqrd() > 1.f)
		moveVector.Normalize();

	viewport->cam.position += moveVector * joystickMovementSpeed * deltaTime;
}