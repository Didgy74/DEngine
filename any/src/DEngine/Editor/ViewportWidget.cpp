#include "ViewportWidget.hpp"

#include <DEngine/Editor/Joystick.hpp>

#include <DEngine/Math/LinearTransform3D.hpp>

#include <DEngine/Gui/StdWidgets/AnchorArea.hpp>

#include <DEngine/GfxGuiDrawEngineImpl.hpp>
#include <DEngine/GuiPlaneComponentSystem.hpp>

import DEngine.Math.Common;
import DEngine.Gui.Utility;

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
		DENGINE_IMPL_ASSERT(Math::Abs(rayDirection.Magnitude() - 1.f) <= 0.00001f);
		DENGINE_IMPL_ASSERT(Math::Abs(planeNormal.Magnitude() - 1.f) <= 0.00001f);

		f32 d = Math::Dot(planeNormal, pointOnPlane);

		// Compute the t value for the directed line ab intersecting the plane
		f32 t = (d - Math::Dot(planeNormal, rayOrigin)) / Math::Dot(planeNormal, rayDirection);
		// If t is above 0, the intersection is in front of the ray, not behind.
		if (t >= 0.0f)
			return Std::Opt{ t };
		else
			return Std::nullOpt;
	}

	struct Rectangle3D {
		Math::Vec3 normal;
		Math::Vec3 center;
		// Rotation around normal in radians.
		f32 rotation;
		f32 width;
		f32 height;
	};
	[[nodiscard]] Std::Opt<f32> Intersect_Ray_Rectangle(
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDirection,
		Rectangle3D const& rect)
	{
		auto const& hitPlane = IntersectRayPlane(rayOrigin, rayDirection, rect.normal, rect.center);
		if (!hitPlane.HasValue())
			return Std::nullOpt;

		auto const& hitDist = hitPlane.Value();

		auto hitPoint = rayOrigin + rayDirection * hitDist;
		auto hitPointRelative = hitPoint - rect.center;

		auto localRightVector = Math::Vec2::Right().GetRotated(rect.rotation);
		
		auto dotWidth = Math::Dot(hitPointRelative, localRightVector.AsVec3());
		auto rectHalfWidth = rect.width / 2.f;

		auto localUpVector = localRightVector.GetRotated90(true);
		auto dotHeight = Math::Dot(hitPointRelative, localUpVector.AsVec3());
		auto rectHalfHeight = rect.height / 2.f;

		if (dotWidth >= -rectHalfWidth && dotWidth <= rectHalfWidth && dotHeight >= -rectHalfHeight && dotHeight <= rectHalfHeight)
			return hitPlane;
		else
			return Std::nullOpt;
	}

	// Returns distance between rayOrigin and intersection point.
	[[nodiscard]] Std::Opt<f32> IntersectRayTri(
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir,
		Math::Vec3 aIn,
		Math::Vec3 bIn,
		Math::Vec3 cIn)
	{
		// Moller-Trumbore Intersection algorithm

		auto const& rayVector = rayDir;

		f32 const EPSILON = 0.0000001f;
		Math::Vec3 vertex0 = aIn;
		Math::Vec3 vertex1 = bIn;
		Math::Vec3 vertex2 = cIn;
		Math::Vec3 edge1, edge2, h, s, q;
		f32 a, f, u, v;
		edge1 = vertex1 - vertex0;
		edge2 = vertex2 - vertex0;
		//h = rayVector.crossProduct(edge2);
		h = Math::Cross(rayVector, edge2);
		//a = edge1.dotProduct(h);
		a = Math::Dot(edge1, h);
		if (a > -EPSILON && a < EPSILON)
			return Std::nullOpt;    // This ray is parallel to this triangle.
		f = 1.0f / a;
		s = rayOrigin - vertex0;
		// u = f * s.dotProduct(h);
		u = f * Math::Dot(s, h);
		if (u < 0.0 || u > 1.0)
			return Std::nullOpt;
		//q = s.crossProduct(edge1);
		q = Math::Cross(s, edge1);
		//v = f * rayVector.dotProduct(q);
		v = f * Math::Dot(rayVector, q);
		if (v < 0.0f || u + v > 1.0f)
			return Std::nullOpt;
		// At this stage we can compute t to find out where the intersection point is on the line.
		//float t = f * edge2.dotProduct(q);
		f32 t = f * Math::Dot(edge2, q);
		if (t > EPSILON){
			return Std::Opt{ t };
		}

		// This means that there is a line intersection but not a ray intersection.
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

		DENGINE_IMPL_ASSERT(Math::Abs(rayDir.Magnitude() - 1.f) <= 0.0001f);

		// Also referred to as "d"
		Math::Vec3 const cylinderAxis = cylinderVertB - cylinderVertA;
		// Vector from cylinder base to ray start.
		Math::Vec3 const m = rayOrigin - cylinderVertA;

		Std::Opt<f32> returnVal = {};

		f32 const md = Math::Dot(m, cylinderAxis);
		f32 const nd = Math::Dot(rayDir, cylinderAxis);
		f32 const dd = cylinderAxis.MagnitudeSqrd();

		f32 const nn = rayDir.MagnitudeSqrd();
		f32 const mn = Math::Dot(m, rayDir);
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
						returnVal = distance;
				}
			}
		}

		return returnVal;
	}

	// World-transform cannot include scale. Bake it into the arrow-struct.
	[[nodiscard]] Std::Opt<f32> IntersectArrow(
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir,
		ViewportGizmo::Arrow arrow,
		Math::Mat4 const& worldTransform) noexcept
	{
		Std::Opt<f32> result;
		auto const vertex1 = (worldTransform * Math::Vec4{ 0, 0, 0, 1 }).AsVec3();
		auto const vertex2 = (worldTransform * Math::Vec4{ 0, 0, arrow.shaftLength, 1 }).AsVec3();
		auto const radius = 0.5f * arrow.shaftDiameter;

		result = IntersectRayCylinder(
			rayOrigin,
			rayDir,
			vertex1,
			vertex2,
			radius);

		return result;
	}

	// Returns the distance of the hit.
	[[nodiscard]] static Std::Opt<f32> Intersect_Ray_PhysicsCollider2D(
		InternalViewportWidget& widget,
		Std::Span<Math::Vec2 const> vertices,
		Math::Vec2 position,
		f32 rotation,
		Math::Vec2 scale,
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir)
	{
		Math::Mat4 transMat = Math::LinAlg3D::Scale_Homo(scale.x, scale.y, 1.f);
		transMat = transMat * Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, rotation);
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

	struct GizmoHitTest_Translate_ReturnT
	{
		ViewportGizmo::GizmoPart gizmoPart;
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
		ViewportGizmo::Arrow arrow = ViewportGizmo::defaultArrow;
		arrow.capLength *= gizmoScale;
		arrow.capDiameter *= gizmoScale;
		arrow.shaftDiameter *= gizmoScale;
		arrow.shaftLength *= gizmoScale;

		auto const localRight = Math::Vec2::Right().GetRotated(gizmoRotation);
		auto const localUp = localRight.GetRotated90(true);
		auto const distToShaftCenter = arrow.shaftLength / 2.f;
		auto const distToCapCenter = arrow.shaftLength + (arrow.capLength / 2.f);

		Std::Opt<f32> closestDistance;
		ViewportGizmo::GizmoPart gizmoPart = {};
		{
			// Next we handle the X arrow

			Rectangle3D shaftRect = {};
			shaftRect.center = gizmoPosition + (localRight * distToShaftCenter).AsVec3();
			shaftRect.normal = Math::Vec3::Forward();
			shaftRect.rotation = gizmoRotation;
			shaftRect.width = arrow.shaftLength;
			shaftRect.height = arrow.shaftDiameter;
			auto const shaftHit = Intersect_Ray_Rectangle(rayOrigin, rayDir, shaftRect);
			if (shaftHit.HasValue() && (!closestDistance.HasValue() || shaftHit.Value() < closestDistance.Value()))
			{
				closestDistance = shaftHit.Value();
				gizmoPart = ViewportGizmo::GizmoPart::ArrowX;
			}

			Rectangle3D capRect = {};
			capRect.center = gizmoPosition + (localRight * distToCapCenter).AsVec3();
			capRect.normal = Math::Vec3::Forward();
			capRect.rotation = gizmoRotation;
			capRect.width = arrow.capLength;
			capRect.height = arrow.capDiameter;
			auto capHit = Intersect_Ray_Rectangle(rayOrigin, rayDir, capRect);
			if (capHit.HasValue() && (!closestDistance.HasValue() || capHit.Value() < closestDistance.Value()))
			{
				closestDistance = capHit.Value();
				gizmoPart = ViewportGizmo::GizmoPart::ArrowX;
			}
		}
		{
			// Next we handle the Y arrow
			Rectangle3D shaftRect = {};
			shaftRect.center = gizmoPosition + (localUp * distToShaftCenter).AsVec3();
			shaftRect.normal = Math::Vec3::Forward();
			shaftRect.rotation = gizmoRotation + Math::pi / 2.f; // Rotate by 90 degrees.
			shaftRect.width = arrow.shaftLength;
			shaftRect.height = arrow.shaftDiameter;
			auto shaftHit = Intersect_Ray_Rectangle(rayOrigin, rayDir, shaftRect);
			if (shaftHit.HasValue() && (!closestDistance.HasValue() || shaftHit.Value() < closestDistance.Value()))
			{
				closestDistance = shaftHit.Value();
				gizmoPart = ViewportGizmo::GizmoPart::ArrowY;
			}

			Rectangle3D capRect = {};
			capRect.center = gizmoPosition + (localUp * distToCapCenter).AsVec3();
			capRect.normal = Math::Vec3::Forward();
			capRect.rotation = gizmoRotation + Math::pi / 2.f; // Rotate by 90 degrees.
			capRect.width = arrow.capLength;
			capRect.height = arrow.capDiameter;
			auto capHit = Intersect_Ray_Rectangle(rayOrigin, rayDir, capRect);
			if (capHit.HasValue() && (!closestDistance.HasValue() || capHit.Value() < closestDistance.Value()))
			{
				closestDistance = capHit.Value();
				gizmoPart = ViewportGizmo::GizmoPart::ArrowY;
			}
		}

		// Then we check the XY quad
		{
			auto planeScale = gizmoScale * ViewportGizmo::defaultPlaneScaleRelative;
			auto planeOffset = gizmoScale * ViewportGizmo::defaultPlaneOffsetRelative;

			Rectangle3D quad = {};
			quad.center = gizmoPosition + Math::Vec2{ 1.f, 1.f }.GetRotated(gizmoRotation).AsVec3() * planeOffset;
			quad.normal = Math::Vec3::Forward();
			quad.rotation = gizmoRotation;
			quad.width = planeScale;
			quad.height = planeScale;
			Std::Opt<f32> hit = Intersect_Ray_Rectangle(rayOrigin, rayDir, quad);

			if (hit.HasValue() && (!closestDistance.HasValue() || hit.Value() < closestDistance.Value()))
			{
				closestDistance = hit.Value();
				gizmoPart = ViewportGizmo::GizmoPart::PlaneXY;
			}
		}

		if (closestDistance.HasValue()) {
			GizmoHitTest_Translate_ReturnT returnVal = {};
			returnVal.distance = closestDistance.Value();
			returnVal.gizmoPart = gizmoPart;
			return returnVal;
		}

		return Std::nullOpt;
	}

	[[nodiscard]] static Std::Opt<GizmoHitTest_Translate_ReturnT> GizmoHitTest_Scale(
		Math::Vec3 gizmoPosition,
		f32 gizmoRotation,
		f32 gizmoScale,
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir) noexcept
	{
		// Gizmo cannot include scale in the world transform, so we modify the arrow struct
		// to account for the scaling
		ViewportGizmo::Arrow arrow = ViewportGizmo::defaultArrow;
		arrow.capLength *= gizmoScale;
		arrow.capDiameter *= gizmoScale;
		arrow.shaftDiameter *= gizmoScale;
		arrow.shaftLength *= gizmoScale;

		auto const localRight = Math::Vec2::Right().GetRotated(gizmoRotation);
		auto const localUp = localRight.GetRotated90(true);
		auto const distToShaftCenter = arrow.shaftLength / 2.f;
		auto const distToCapCenter = arrow.shaftLength + (arrow.capLength / 2.f);

		Std::Opt<f32> closestDistance;
		ViewportGizmo::GizmoPart gizmoPart = {};
		{
			// Next we handle the X arrow

			Rectangle3D shaftRect = {};
			shaftRect.center = gizmoPosition + (localRight * distToShaftCenter).AsVec3();
			shaftRect.normal = Math::Vec3::Forward();
			shaftRect.rotation = gizmoRotation;
			shaftRect.width = arrow.shaftLength;
			shaftRect.height = arrow.shaftDiameter;
			auto shaftHit = Intersect_Ray_Rectangle(rayOrigin, rayDir, shaftRect);
			if (shaftHit.HasValue() && (!closestDistance.HasValue() || shaftHit.Value() < closestDistance.Value()))
			{
				closestDistance = shaftHit.Value();
				gizmoPart = ViewportGizmo::GizmoPart::ArrowX;
			}

			Rectangle3D capRect = {};
			capRect.center = gizmoPosition + (localRight * distToCapCenter).AsVec3();
			capRect.normal = Math::Vec3::Forward();
			capRect.rotation = gizmoRotation;
			capRect.width = arrow.capLength;
			capRect.height = arrow.capLength;
			auto capHit = Intersect_Ray_Rectangle(rayOrigin, rayDir, capRect);
			if (capHit.HasValue() && (!closestDistance.HasValue() || capHit.Value() < closestDistance.Value()))
			{
				closestDistance = capHit.Value();
				gizmoPart = ViewportGizmo::GizmoPart::ArrowX;
			}
		}
		{
			// Next we handle the Y arrow

			Rectangle3D shaftRect = {};
			shaftRect.center = gizmoPosition + (localUp * distToShaftCenter).AsVec3();
			shaftRect.normal = Math::Vec3::Forward();
			shaftRect.rotation = gizmoRotation + Math::pi / 2.f; // Rotate by 90 degrees.
			shaftRect.width = arrow.shaftLength;
			shaftRect.height = arrow.shaftDiameter;
			auto shaftHit = Intersect_Ray_Rectangle(rayOrigin, rayDir, shaftRect);
			if (shaftHit.HasValue() && (!closestDistance.HasValue() || shaftHit.Value() < closestDistance.Value()))
			{
				closestDistance = shaftHit.Value();
				gizmoPart = ViewportGizmo::GizmoPart::ArrowY;
			}

			Rectangle3D capRect = {};
			capRect.center = gizmoPosition + (localUp * distToCapCenter).AsVec3();
			capRect.normal = Math::Vec3::Forward();
			capRect.rotation = gizmoRotation + Math::pi / 2.f; // Rotate by 90 degrees.
			capRect.width = arrow.capLength;
			capRect.height = arrow.capLength;
			auto capHit = Intersect_Ray_Rectangle(rayOrigin, rayDir, capRect);
			if (capHit.HasValue() && (!closestDistance.HasValue() || capHit.Value() < closestDistance.Value()))
			{
				closestDistance = capHit.Value();
				gizmoPart = ViewportGizmo::GizmoPart::ArrowY;
			}
		}

		// Then we check the XY quad
		{
			auto planeScale = gizmoScale * ViewportGizmo::defaultPlaneScaleRelative;
			auto planeOffset = gizmoScale * ViewportGizmo::defaultPlaneOffsetRelative;

			Rectangle3D quad = {};
			quad.center = gizmoPosition + Math::Vec2{ 1.f, 1.f }.GetRotated(gizmoRotation).AsVec3() * planeOffset;
			quad.normal = Math::Vec3::Forward();
			quad.rotation = gizmoRotation;
			quad.width = planeScale;
			quad.height = planeScale;
			Std::Opt<f32> hit = Intersect_Ray_Rectangle(rayOrigin, rayDir, quad);

			if (hit.HasValue() && (!closestDistance.HasValue() || hit.Value() < closestDistance.Value()))
			{
				closestDistance = hit.Value();
				gizmoPart = ViewportGizmo::GizmoPart::PlaneXY;
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
		constexpr f32 outerCircleRadius = ViewportGizmo::defaultRotateCircleOuterRadius;
		constexpr f32 innerCircleRadius = ViewportGizmo::defaultRotateCircleInnerRadius;
		f32 const innerDistance = (outerCircleRadius - innerCircleRadius) * scale;
		f32 const outerDistance = (outerCircleRadius + innerCircleRadius) * scale;

		if (relativeDistSqrd >= Math::Sqrd(innerDistance) && relativeDistSqrd <= Math::Sqrd(outerDistance))
			return hitPointDistOpt;
		else
			return {};
	}

	struct GizmoHitTest_ReturnT
	{
		// Holds the hit point relative to the position. No scaling or rotation applied.
		Math::Vec3 relativeHitPoint_Object;
		ViewportGizmo::GizmoPart part;
		// Holds the normalized 
		Math::Vec3 normalizedHitPoint_Gizmo;
		f32 rotationOffset;
	};

	[[nodiscard]] static Std::Opt<GizmoHitTest_ReturnT> GizmoHitTest(
		InternalViewportWidget const& widget,
		Gui::Rect const& widgetRect,
		Math::Vec2 pointerPos,
		Math::Vec3 gizmoPosition,
		f32 gizmoRotation,
		f32 gizmoTargetSizePx,
		GizmoType gizmoType)
	{
		Math::Mat4 worldTransform = Math::Mat4::Identity();
		Math::LinAlg3D::SetTranslation(worldTransform, gizmoPosition);

		auto const scale = ViewportGizmo::ComputeScale(
			worldTransform,
			gizmoTargetSizePx,
			widget.BuildProjectionMatrix(widgetRect.extent.Aspect()),
			widgetRect.extent);

		auto const rayOrigin = widget.cam.position;
		auto const rayDir = widget.BuildRayDirection(widgetRect, pointerPos);

		if (gizmoType == GizmoType::Translate)
		{
			auto const& hitOpt = GizmoHitTest_Translate(
				gizmoPosition,
				gizmoRotation,
				scale,
				rayOrigin,
				rayDir);
			if (hitOpt.HasValue())
			{
				auto const& hit = hitOpt.Value();
				auto const hitPoint = rayOrigin + rayDir * hit.distance;

				GizmoHitTest_ReturnT returnVal = {};
				returnVal.part = hit.gizmoPart;

				// Find the point we hit relative to the gizmo.
				// Then scale it down to make it normalized [-1, 1]
				returnVal.normalizedHitPoint_Gizmo = hitPoint - gizmoPosition;
				returnVal.normalizedHitPoint_Gizmo *= 1.f / scale;

				return returnVal;
			}
		}
		else if (gizmoType == GizmoType::Rotate)
		{
			auto const& hitOpt = GizmoHitTest_Rotate(gizmoPosition, scale, rayOrigin, rayDir);
			if (hitOpt.HasValue())
			{
				auto const& distance = hitOpt.Value();
				GizmoHitTest_ReturnT returnVal = {};
				auto const absoluteHitPoint = rayOrigin + (rayDir * hitOpt.Value());
				auto const relativeHitPoint = absoluteHitPoint - gizmoPosition;
				auto const localUp = Math::Vec2::Up().GetRotated(gizmoRotation);
				returnVal.rotationOffset = Math::Vec2::SignedAngle(localUp, relativeHitPoint.AsVec2().GetNormalized());
				return returnVal;
			}
		}
		else if (gizmoType == GizmoType::Scale)
		{
			auto const& hitOpt = GizmoHitTest_Scale(
				gizmoPosition,
				gizmoRotation,
				scale,
				rayOrigin,
				rayDir);
			if (hitOpt.HasValue())
			{
				auto const& hit = hitOpt.Value();
				auto const hitPoint = rayOrigin + rayDir * hit.distance;

				GizmoHitTest_ReturnT returnVal = {};
				returnVal.part = hit.gizmoPart;

				returnVal.relativeHitPoint_Object = hitPoint - gizmoPosition;

				return returnVal;
			}
		}
		else
			DENGINE_IMPL_UNREACHABLE();

		return Std::nullOpt;
	}

	static void Gizmo_HandleTranslation(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		ViewportWidget::HoldingGizmoData const& gizmoHoldingData,
		Math::Vec2 pointerPos,
		f32 gizmoTargetSizePx,
		Transform& transform)
	{
		auto const rayOrigin = widget.cam.position;
		auto const rayDir = widget.BuildRayDirection(widgetRect, pointerPos);

		auto const transformMat = Math::LinAlg3D::Translate(transform.position);
		auto const scale = ViewportGizmo::ComputeScale(
			transformMat,
			gizmoTargetSizePx,
			widget.BuildProjectionMatrix(widgetRect.extent.Aspect()),
			widgetRect.extent);

		auto newPos = gizmoHoldingData.initialPos;

		auto const gizmoOffset = gizmoHoldingData.normalizedOffsetGizmo * scale;
		auto const hit = IntersectRayPlane(
			rayOrigin, 
			rayDir, 
			Math::Vec3::Forward(), 
			gizmoHoldingData.initialPos - gizmoOffset);

		if (hit.HasValue())
		{
			auto posOnPlane = (rayOrigin + rayDir * hit.Value()) - gizmoOffset;
			newPos = posOnPlane;
			if (gizmoHoldingData.holdingPart == ViewportGizmo::GizmoPart::ArrowX || gizmoHoldingData.holdingPart == ViewportGizmo::GizmoPart::ArrowY)
			{
				Math::Vec3 movementAxis = {};
				switch (gizmoHoldingData.holdingPart)
				{
				case ViewportGizmo::GizmoPart::ArrowX:
					movementAxis = Math::Vec2::Right()
						.GetRotated(Math::degToRad * transform.rotation.ToEulerAngles().z)
						.AsVec3();
					break;
				case ViewportGizmo::GizmoPart::ArrowY:
					movementAxis = Math::Vec2::Up()
						.GetRotated(Math::degToRad * transform.rotation.ToEulerAngles().z)
						.AsVec3();
					break;
				default:
					DENGINE_IMPL_UNREACHABLE();
					break;
				}
				newPos = gizmoHoldingData.initialPos + movementAxis * Math::Dot(movementAxis, newPos - gizmoHoldingData.initialPos);
			}
		}

		transform.position = newPos;
	}

	static void Gizmo_HandleRotation(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		Math::Vec2 pointerPos,
		Math::UnitQuat const& initialRotation,
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

		if (!distanceOpt.HasValue()) {
			return;
		}

		auto const hitPoint = rayOrigin + rayDir * distanceOpt.Value();
		auto const hitPointRel = hitPoint - transform.position;

		// We should technically project the hitpoint as 2D vector onto the plane,
		// But the plane is facing directly towards Z anyways so we just ditch the Z component.

		Math::Vec2 temp = hitPointRel.AsVec2().GetNormalized();
		// Absolute target Z angle (radians) reconstructed from pointer + grab-time offset.
		f32 const targetZ = Math::Vec2::SignedAngle(temp, Math::Vec2::Up()) + rotationOffset;
		// FromEulerAngles uses YXZ order (Qy * Qx * Qz), so Z is the innermost rotation.
		// Right-multiplying initialRotation by a Z-delta cleanly replaces only the Z component
		// without round-tripping X/Y through Euler.
		f32 const initialZ = Math::degToRad * initialRotation.ToEulerAngles().z;
		auto const deltaZ = Math::UnitQuat::FromVector(Math::Vec3::Forward(), targetZ - initialZ);
		transform.rotation = initialRotation * deltaZ;
	}

	static void Gizmo_HandleScaling(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		ViewportWidget::HoldingGizmoData const& holdingData,
		Math::Vec2 pointerPos,
		f32 gizmoTargetSizePx,
		Transform& transform)
	{
		auto const rayOrigin = widget.cam.position;
		auto const rayDir = widget.BuildRayDirection(widgetRect, pointerPos);

		auto transformMat = Math::LinAlg3D::Translate(transform.position);
		auto const scale = ViewportGizmo::ComputeScale(
			transformMat,
			gizmoTargetSizePx,
			widget.BuildProjectionMatrix(widgetRect.extent.Aspect()),
			widgetRect.extent);
		
		// First raycast against the plane of the object.
		auto pointDist = IntersectRayPlane(rayOrigin, rayDir, Math::Vec3::Forward(), transform.position);
		if (pointDist.HasValue())
		{
			auto const newRelativePoint = (rayOrigin + rayDir * pointDist.Value()) - transform.position;

			switch (holdingData.holdingPart)
			{
				case ViewportGizmo::GizmoPart::ArrowX:
				{
					f32 const zRot = Math::degToRad * transform.rotation.ToEulerAngles().z;
					auto const x = Math::Vec2::Right().GetRotated(zRot).AsVec3();
					auto const dotX = Math::Dot(holdingData.relativeHitPointObject, x) / holdingData.initialObjectScale.x;
					auto const newDotX = Math::Dot(newRelativePoint, x);

					transform.scale.x = newDotX / dotX;
					break;
				}
				case ViewportGizmo::GizmoPart::ArrowY:
				{
					f32 const zRot = Math::degToRad * transform.rotation.ToEulerAngles().z;
					auto const y = Math::Vec2::Up().GetRotated(zRot).AsVec3();
					auto const dotY = Math::Dot(holdingData.relativeHitPointObject, y) / holdingData.initialObjectScale.y;
					auto const newDotY = Math::Dot(newRelativePoint, y);

					transform.scale.y = newDotY / dotY;
					break;
				}
				case ViewportGizmo::GizmoPart::PlaneXY:
				{
					auto const x = Math::Vec2::Right().AsVec3();
					auto const dotX = Math::Dot(holdingData.relativeHitPointObject, x) / holdingData.initialObjectScale.x;
					auto const newDotX = Math::Dot(newRelativePoint, x);

					transform.scale.x = newDotX / dotX;

					transform.scale.y = transform.scale.x * holdingData.initialObjectScale.y / holdingData.initialObjectScale.x;
					break;
				}
			}
		}
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

	// This viewport's pressed-pointer set: the ids of pointers whose primary press landed in this
	// viewport and have not yet been released. Stays tiny and linear - at most a handful of fingers
	// plus the cursor are ever down on one viewport at once.
	static void AddPressedPointer(InternalViewportWidget& widget, u8 pointerId) {
		for (auto const id : widget.pressedPointerIds) {
			if (id == pointerId)
				return;
		}
		widget.pressedPointerIds.PushBack(pointerId);
	}
	static void RemovePressedPointer(InternalViewportWidget& widget, u8 pointerId) {
		auto& ids = widget.pressedPointerIds;
		for (uSize i = 0; i < ids.Size(); i += 1) {
			if (ids[i] == pointerId) {
				ids.EraseUnsorted(i);
				return;
			}
		}
	}
	[[nodiscard]] static bool IsPointerPressedHere(InternalViewportWidget const& widget, u8 pointerId) {
		for (auto const id : widget.pressedPointerIds) {
			if (id == pointerId)
				return true;
		}
		return false;
	}

	struct PointerMove_Pointer {
		Math::Vec2 pos;
		Math::Vec2 posDelta;
		u8 id;
		bool occluded;
	};
	struct PointerMove_Params {
		InternalViewportWidget& widget;
		Gui::RectCollection const& rectColl;
		Std::Opt<Gui::RectCollection::Iter> const& rectCollIter;
		PointerMove_Pointer const& pointer;
        Std::AnyRef customData;
		Gui::TextEngine& textEngine;
		// On-screen size in pixels the gizmo should target, derived from the window's
		// minimum interactable height. Computed by the dispatcher and discarded after use.
		f32 gizmoTargetSizePx;
	};
	[[nodiscard]] static Gui::TouchEventConsumption PointerMove(PointerMove_Params const& params) {
		auto& widget = params.widget;
		auto const& rectColl = params.rectColl;
		auto const& rectCollIter = params.rectCollIter;
		auto const& pointer = params.pointer;

        auto* appDataPtr = params.customData.Get<Editor::EditorGuiAppData>();
        DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
        auto& editorCtx = appDataPtr->EditorCtx();

		auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
		DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
		auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
		auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

		bool pointerInside = Gui::PointIsInAll(pointer.pos, { absWidgetRect, absVisibleRect });

		if (widget.state == ViewportWidget::BehaviorState::Gizmo) {
			DENGINE_IMPL_ASSERT(widget.holdingGizmoData.HasValue());
			Entity entity = {};
			Transform* transformPtr = nullptr;
			auto const& gizmoHoldingData = widget.holdingGizmoData.Value();
			if (gizmoHoldingData.pointerId == pointer.id && editorCtx.GetSelectedEntity().HasValue())
			{
				entity = editorCtx.GetSelectedEntity().Value();
				// First check if we have a transform
				Scene& scene = editorCtx.GetActiveScene();
				transformPtr = scene.GetComponent<Transform>(entity);
			}
			if (transformPtr != nullptr)
			{
				auto& transform = *transformPtr;
				auto const currGizmo = gizmoHoldingData.gizmoType;
				if (currGizmo == GizmoType::Translate) {
					impl::Gizmo_HandleTranslation(
						widget,
						absWidgetRect,
						gizmoHoldingData,
						pointer.pos,
						params.gizmoTargetSizePx,
						transform);
				}
				else if (currGizmo == GizmoType::Rotate)
				{
					impl::Gizmo_HandleRotation(
						widget,
						absWidgetRect,
						pointer.pos,
						gizmoHoldingData.initialRotation,
						gizmoHoldingData.rotationOffset,
						transform);
				}
				else if (currGizmo == GizmoType::Scale)
				{
					impl::Gizmo_HandleScaling(
						widget,
						absWidgetRect,
						gizmoHoldingData,
						pointer.pos,
						params.gizmoTargetSizePx,
						transform);
				}
				else
					DENGINE_IMPL_UNREACHABLE();
			}
		}

		if (widget.state == ViewportWidget::BehaviorState::FreeLooking &&
			pointer.id == cursorPointerId)
		{
			f32 sensitivity = 0.2f;
			Math::Vec2 amount = { pointer.posDelta.x, -pointer.posDelta.y };
			widget.ApplyCameraRotation(amount * sensitivity * Math::degToRad);
		}

		Scene& scene = editorCtx.GetActiveScene();
		bool const heldByScene = GuiPlaneComponentSystem::IsPointerHeld(scene, pointer.id);
		bool const pressedHere = impl::IsPointerPressedHere(widget, pointer.id);
		// A captured pointer keeps feeding the plane it grabbed even after the ray leaves the plane
		// bounds - but only through the viewport it was pressed on. Every viewport sharing this scene
		// receives every move event, so without the pressedHere gate they would each re-forward the
		// same captured pointer from their own ray and corrupt the interaction.
		bool const dragFollow = heldByScene && pressedHere;
		// Genuine hover (e.g. the cursor passing over a plane). A pointer captured by some viewport
		// belongs to that viewport, never to whichever one it happens to cross, so a held pointer is
		// excluded here and handled solely by dragFollow above.
		bool const hoverInViewport =
			!pointer.occluded &&
			pointerInside &&
			!heldByScene &&
			widget.state == ViewportWidget::BehaviorState::Normal;
		if (hoverInViewport || dragFollow) {
			Math::Vec3 const rayOrigin = widget.cam.position;
			Math::Vec3 const rayDir = widget.BuildRayDirection(absWidgetRect, pointer.pos);

			// GuiPlane widgets expect the scene as their app-data; do NOT forward the viewport's
			// customData here (that's Editor::Context, a different type).
			if (pointer.id == cursorPointerId) {
				GuiPlaneComponentSystem::DispatchCursorMoveFromRay_Params dispatchParams = {
					.scene = scene,
					.textEngine = params.textEngine,
					.rayOrigin = rayOrigin,
					.rayDir = rayDir };
				GuiPlaneComponentSystem::DispatchCursorMoveFromRay(dispatchParams);
			} else {
				GuiPlaneComponentSystem::DispatchTouchMoveFromRay_Params dispatchParams = {
					.scene = scene,
					.textEngine = params.textEngine,
					.rayOrigin = rayOrigin,
					.rayDir = rayDir,
					.fingerId = pointer.id };
				GuiPlaneComponentSystem::DispatchTouchMoveFromRay(dispatchParams);
			}
		}

		Gui::TouchEventConsumption returnVal = {};
		if (pointerInside || dragFollow) {
			returnVal.consumed = true;
			returnVal.claimDragPriority = true;
		}
		return returnVal;
	}
}

using namespace DEngine;
using namespace DEngine::Editor;

namespace DEngine::Editor::impl {
	struct PointerPress_Pointer {
		Math::Vec2 pos;
		u8 id;
		bool pressed;
		PointerType type;
	};
	struct PointerPress_Params {
		Std::AnyRef customData;
		bool eventConsumed;
		PointerPress_Pointer const& pointer;
		Gui::RectCollection const& rectColl;
		Std::Opt<Gui::RectCollection::Iter> const& rectCollIter;
		Gui::TextEngine& textEngine;
		Gui::AllocRef transientAlloc;
		InternalViewportWidget& widget;
		// On-screen size in pixels the gizmo should target, derived from the window's
		// minimum interactable height. Computed by the dispatcher and discarded after use.
		f32 gizmoTargetSizePx;
	};
	[[nodiscard]] static Gui::TouchEventConsumption PointerPress(PointerPress_Params const& params) {
		auto& widget = params.widget;
		auto const& rectColl = params.rectColl;
		auto const& rectCollIter = params.rectCollIter;
		auto const& pointer = params.pointer;
		auto const& oldEventConsumed = params.eventConsumed;

        auto* appDataPtr = params.customData.Get<Editor::EditorGuiAppData>();
        DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
        auto& editorCtx = appDataPtr->EditorCtx();

		auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
		DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
		auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
		auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

		bool pointerInside = Gui::PointIsInAll(pointer.pos, { absWidgetRect, absVisibleRect });

		if (!pointerInside && pointer.pressed) {
			return {};
		}

		// Snapshot ownership before RemovePressedPointer below clears it; releaseHeldPointer needs it.
		bool const wasPressedHere = impl::IsPointerPressedHere(widget, pointer.id);

		// A release ends this viewport's ownership of the pointer wherever it lands (a drag often
		// releases off-viewport). Done before the early-outs below so no release path leaks an id.
		// Gated to Primary so a secondary-button release can't drop an in-progress primary drag,
		// since the cursor shares one id across its buttons.
		if (!pointer.pressed && pointer.type == PointerType::Primary) {
			impl::RemovePressedPointer(widget, pointer.id);
		}

		if (widget.holdingGizmoData.HasValue() &&
			widget.holdingGizmoData.Value().pointerId == pointer.id &&
			!pointer.pressed)
		{
			widget.holdingGizmoData = Std::nullOpt;
			widget.state = ViewportWidget::BehaviorState::Normal;
			return {};
		}

		// Initiate free-look
		if (!oldEventConsumed &&
			pointerInside &&
			pointer.pressed &&
			widget.state == ViewportWidget::BehaviorState::Normal &&
			pointer.type == PointerType::Secondary)
		{
			widget.state = ViewportWidget::BehaviorState::FreeLooking;
			widget.editorImpl->appCtx->LockCursor(true);
			return { .consumed = true, .claimDragPriority = true };
		}

		// Stop free-look
		if (!pointer.pressed &&
			widget.state == ViewportWidget::BehaviorState::FreeLooking &&
			pointer.type == PointerType::Secondary)
		{
			widget.state = ViewportWidget::BehaviorState::Normal;
            widget.editorImpl->appCtx->LockCursor(true);
			return {};
		}

		Scene& scene = editorCtx.GetActiveScene();

		// Only the viewport that owned the press may drive the release. IsPointerHeld is scene-global,
		// so without wasPressedHere a viewport that never captured this pointer would also fire the
		// plane's release from its own ray - at the wrong coords, and ending the capture early.
		bool const releaseHeldPointer =
			!pointer.pressed &&
			pointer.type == PointerType::Primary &&
			wasPressedHere &&
			GuiPlaneComponentSystem::IsPointerHeld(scene, pointer.id);

		// Press/release in the viewport with the primary pointer feeds three things in order:
		// 1) Gizmo hit-test (press only) — grab a handle for drag.
		// 2) GuiPlane press (press and release) — forward to the plane's embedded widget.
		// 3) Entity-collider select (press only) — pick an entity in the scene.
		bool const primaryInViewport =
			!oldEventConsumed &&
			pointerInside &&
			widget.state == ViewportWidget::BehaviorState::Normal &&
			pointer.type == PointerType::Primary;

		// This viewport now owns the pointer's press. Combined with the scene's capture state, the
		// pressed set is what lets only this viewport drive a plane the pointer goes on to grab.
		if (primaryInViewport && pointer.pressed) {
			impl::AddPressedPointer(widget, pointer.id);
		}

		if (primaryInViewport && pointer.pressed) {
			// Gizmo handle hit-test for the currently selected entity.
			Entity ent = Entity::Invalid;
			Transform const* transformPtr = nullptr;
			if (editorCtx.GetSelectedEntity().HasValue()) {
				ent = editorCtx.GetSelectedEntity().Value();
				transformPtr = scene.GetComponent<Transform>(ent);
			}
			if (transformPtr != nullptr) {
				Transform const& transform = *transformPtr;
				auto const currGizmoType = editorCtx.GetCurrentGizmoType();
				auto const& hitTestOpt = impl::GizmoHitTest(
					widget,
					absWidgetRect,
					pointer.pos,
					transform.position,
					Math::degToRad * transform.rotation.ToEulerAngles().z,
					params.gizmoTargetSizePx,
					currGizmoType);
				if (hitTestOpt.HasValue()) {
					auto const& hit = hitTestOpt.Value();
					ViewportWidget::HoldingGizmoData holdingGizmoData = {};
					holdingGizmoData.gizmoType = currGizmoType;
					holdingGizmoData.holdingPart = hit.part;
					holdingGizmoData.initialPos = transform.position;
					holdingGizmoData.initialRotation = transform.rotation;
					holdingGizmoData.normalizedOffsetGizmo = hit.normalizedHitPoint_Gizmo;
					holdingGizmoData.pointerId = pointer.id;
					holdingGizmoData.initialObjectScale = transform.scale;
					holdingGizmoData.relativeHitPointObject = hit.relativeHitPoint_Object;
					holdingGizmoData.rotationOffset = hit.rotationOffset;
					widget.holdingGizmoData = holdingGizmoData;
					widget.state = ViewportWidget::BehaviorState::Gizmo;

					return { .consumed = true, .claimDragPriority = true };
				}
			}
		}

		if (primaryInViewport || releaseHeldPointer) {
			// Forward the press/release into the scene. A press hit-tests within plane bounds and
			// captures the pointer onto the hit plane; a release follows that capture (bounds
			// ignored) and ends it. Runs for an in-viewport press/release, and also for the release
			// of an already-held pointer that lands outside the viewport - otherwise the capture, and
			// the in-scene gesture it drives (e.g. a ScrollArea touch-scroll), never sees its release.
			Math::Vec3 const rayOrigin = widget.cam.position;
			Math::Vec3 const rayDir = widget.BuildRayDirection(absWidgetRect, pointer.pos);

			// GuiPlane widgets expect the scene as their app-data — they read it via
			// customData.Get<Scene>() in their event handlers. Do NOT forward the
			// viewport's customData here (that's Editor::Context, a different type).
			bool hitPlane;

			if (pointer.id == cursorPointerId) {
				GuiPlaneComponentSystem::DispatchCursorPressFromRay_Params dispatchParams = {
					.scene = scene,
					.textEngine = params.textEngine,
					.platformCtx = *widget.editorImpl->appCtx,
					.transientAlloc = params.transientAlloc,
					.rayOrigin = rayOrigin,
					.rayDir = rayDir,
					.button = Gui::CursorButton::Primary,
					.pressed = pointer.pressed };
				hitPlane = GuiPlaneComponentSystem::DispatchCursorPressFromRay(dispatchParams);
			} else {
				GuiPlaneComponentSystem::DispatchTouchPressFromRay_Params dispatchParams = {
					.scene = scene,
					.textEngine = params.textEngine,
					.platformCtx = *widget.editorImpl->appCtx,
					.rayOrigin = rayOrigin,
					.rayDir = rayDir,
					.fingerId = pointer.id,
					.pressed = pointer.pressed };
				hitPlane = GuiPlaneComponentSystem::DispatchTouchPressFromRay(dispatchParams);
			}
			if (hitPlane) {
				return { .consumed = true, .claimDragPriority = true };
			}
		}

		if (primaryInViewport && pointer.pressed) {
			// Entity-collider hit-test for selection.
			// Collapse this shit into a function
			Std::Opt<Std::Pair<f32, Entity>> hitEntity;

			Math::Vec3 rayOrigin = widget.cam.position;
			Math::Vec3 rayDir = widget.BuildRayDirection(absWidgetRect, pointer.pos);
			// Iterate over all physics components that also have a transform component
			for (auto const& [entity, rb] : scene.GetAllComponents<Physics::Rigidbody2D>())
			{
				Transform const* transform = editorCtx.GetActiveScene().GetComponent<Transform>(entity);
				if (transform)
				{
					Math::Vec2 vertices[4] = {
						{-0.5f, 0.5f },
						{ 0.5f, 0.5f },
						{ 0.5f, -0.5f },
						{ -0.5f, -0.5f } };
					Std::Opt<f32> distanceOpt = Intersect_Ray_PhysicsCollider2D(
						widget,
						{ vertices, 4 },
						transform->position.AsVec2(),
						Math::degToRad * transform->rotation.ToEulerAngles().z,
						transform->scale,
						rayOrigin,
						rayDir);
					if (distanceOpt.HasValue())
					{
						auto const newDist = distanceOpt.Value();
						if (!hitEntity.HasValue() || newDist <= hitEntity.Value().a)
							hitEntity = { newDist, entity };
					}
				}
			}
			if (hitEntity.HasValue()) {
				editorCtx.SelectEntity(hitEntity.Value().b);
				return { .consumed = true, .claimDragPriority = true };
			}
		}

		return { .consumed = pointerInside };
	}
}

f32 ViewportGizmo::ComputeTargetSizePx(f32 minimumHeightCm, f32 dpi) noexcept
{
	// "Width of the arrows" is the shaft's on-screen thickness, which we want to match the
	// window's minimum interactable height so the handle stays comfortably grabbable. On a
	// unit-length gizmo the shaft is only `shaftDiameter` units thick, so the whole gizmo
	// must span minHeight / shaftDiameter for the shaft itself to equal minHeight. With the
	// default 0.1 shaft this is the natural 10x factor.
	auto const minHeightPx = (f32)Gui::CmToPixels(minimumHeightCm, dpi);
	return minHeightPx / ViewportGizmo::defaultArrow.shaftDiameter;
}

[[nodiscard]] f32 ViewportGizmo::ComputeScale(
	Math::Mat4 const& worldTransform,
	f32 targetSizePx,
	Math::Mat4 const& projection,
	Gui::Extent viewportSize) noexcept
{
	auto const pixelSize = 1.f / (f32)viewportSize.height;
	Math::Vec4 zVec = { worldTransform.At(3, 0), worldTransform.At(3, 1), worldTransform.At(3, 2), worldTransform.At(3, 3) };
	return  targetSizePx * pixelSize * (projection * zVec).w * gizmoScaleMultiplier;
}

InternalViewportWidget::InternalViewportWidget(
	EditorImpl& implData) :
	editorImpl(&implData)
{
	auto newViewportRef = implData.gfxCtx->NewViewport();
	viewportId = newViewportRef.ViewportID();
}

InternalViewportWidget::~InternalViewportWidget()
{
	editorImpl->gfxCtx->DeleteViewport(viewportId);
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
	DENGINE_IMPL_ASSERT(perspMatInvOpt.HasValue());
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
	f32 dot = Math::Dot(forward, Math::Vec3::Up());
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
	if (!currentlyResizing && currentExtent != newExtent) {
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

Gfx::ViewportUpdate InternalViewportWidget::BuildViewportUpdate(
	std::vector<Math::Vec3>& lineVertices,
	std::vector<Gfx::LineDrawCmd>& lineDrawCmds) const noexcept
{
	DENGINE_IMPL_ASSERT(editorImpl);
	Scene const& scene = editorImpl->GetActiveScene();

	Gfx::ViewportUpdate returnVal = {};
	returnVal.id = viewportId;
	returnVal.width = currentExtent.width;
	returnVal.height = currentExtent.height;
	returnVal.clearColor = Editor::Settings::GetColor(Editor::Settings::Color::Window_DefaultViewportBackground);

	f32 aspectRatio = (f32)newExtent.width / (f32)newExtent.height;
	Math::Mat4 projMat = BuildProjectionMatrix(aspectRatio);

	returnVal.transform = projMat;

	if (editorImpl->GetSelectedEntity().HasValue()) {
		Entity selected = editorImpl->GetSelectedEntity().Value();
		// Find Transform component of this entity

		auto transformPtr = scene.GetComponent<Transform>(selected);

		// Draw gizmo
		if (transformPtr != nullptr) {
			Transform const& transform = *transformPtr;

			returnVal.gizmoOpt = Gfx::ViewportUpdate::Gizmo{};
			Gfx::ViewportUpdate::Gizmo& gizmo = returnVal.gizmoOpt.Value();
			gizmo.type = impl::ToGfxGizmoType(editorImpl->GetCurrentGizmoType());
			gizmo.position = transform.position;
			gizmo.rotation = Math::degToRad * transform.rotation.ToEulerAngles().z;

			// The gizmo's wanted pixel size was derived from the window during the render pass
			// and stashed per-viewport; recover it here so the gizmo respects the minimum height.
			// The depth-dependent scale itself is still computed fresh so perspective stays correct.
			f32 gizmoTargetSizePx = 0;
			for (auto const& item : editorImpl->renderResultData.viewports) {
				if (item.a == viewportId) {
					gizmoTargetSizePx = item.b.gizmoTargetSizePx;
					break;
				}
			}

			Math::Mat4 worldTransform = Math::Mat4::Identity();
			Math::LinAlg3D::SetTranslation(worldTransform, { transform.position.x, transform.position.y, 0.f });
			f32 scale = ViewportGizmo::ComputeScale(
				worldTransform,
				gizmoTargetSizePx,
				projMat,
				newExtent);
			gizmo.scale = scale;

			gizmo.quadScale = gizmo.scale * ViewportGizmo::defaultPlaneScaleRelative;
			gizmo.quadOffset = gizmo.scale * ViewportGizmo::defaultPlaneOffsetRelative;
		}

		// Draw debug lines for collider if there is one
		auto const rbPtr = scene.GetComponent<Physics::Rigidbody2D>(selected);
		if (rbPtr && transformPtr) {
			auto const& transform = *transformPtr;

			Math::Vec2 vertices[4] = {
				{-0.5f, 0.5f },
				{ 0.5f, 0.5f },
				{ 0.5f, -0.5f },
				{ -0.5f, -0.5f } };

			Math::Mat4 worldTransform = Math::Mat4::Identity();
			worldTransform = Math::LinAlg3D::Scale_Homo(transform.scale.AsVec3()) * worldTransform;
			worldTransform = Math::LinAlg3D::Rotate_Homo(transform.rotation) * worldTransform;
			Math::LinAlg3D::SetTranslation(worldTransform, transform.position);

			// Iterate over the 4 points of the box.
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

Gui::Widget::GetSizeHint2_ReturnT InternalViewportWidget::GetSizeHint2(
	Gui::Widget::GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;

	Gui::SizeHint returnVal = {};
	returnVal.minimum = { 450, 450 };
	returnVal.expandX = true;
	returnVal.expandY = true;

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnVal, false);

	return {
		.iter = entry,
		.sizeHint = returnVal };
}

void InternalViewportWidget::Render2(
	Render_Params const& params,
	Std::Opt<Gui::RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectCollection;
	auto& drawInfo = static_cast<GfxGuiDrawEngineImpl&>(params.drawEngine);

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	if (absWidgetRect.GetIntersect(rectPairOpt.Value().visibleRect).IsNothing()) {
		return;
	}

	drawInfo.PushViewport(absWidgetRect, this->viewportId);

	auto* appDataPtr = params.appData.Get<EditorGuiAppData>();
	DENGINE_IMPL_GUI_ASSERT(appDataPtr != nullptr);
	auto* resultDataPtr = appDataPtr->RenderResultData();
	DENGINE_IMPL_GUI_ASSERT(resultDataPtr != nullptr);
	auto& resultData = *resultDataPtr;

	resultData.viewports.push_back({ this->viewportId,
		EditorGuiAppData_Render_Temp::ViewportWidgetRenderData {
			.gizmoTargetSizePx = ViewportGizmo::ComputeTargetSizePx(
				params.window.minimumHeightCm,
				params.window.dpi) } });
}

bool InternalViewportWidget::CursorPress2(
	CursorPressParams const& params,
	Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.id = impl::cursorPointerId,
		.pressed = params.CursorPressed(),
		.type = impl::ToPointerType(params.CursorButton()) };

	// Grab the Scene and forward it.

	impl::PointerPress_Params tempParams = {
		.customData = params.customData,
		.eventConsumed = consumed,
		.pointer = pointer,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.textEngine = params.textEngine,
		.transientAlloc = params.transientAlloc,
		.widget = *this,
		.gizmoTargetSizePx = ViewportGizmo::ComputeTargetSizePx(
			params.window.minimumHeightCm,
			params.window.dpi), };
	auto temp = impl::PointerPress(tempParams);
	return temp.consumed;
}

bool InternalViewportWidget::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto const& rectColl = params.rectCollection;

	impl::PointerMove_Pointer pointer = {
		.pos = { (f32)params.CursorPos().x, (f32)params.CursorPos().y },
		.posDelta = { (f32)params.CursorPosDelta().x, (f32)params.CursorPosDelta().y },
		.id = impl::cursorPointerId,
		.occluded = occluded, };

	impl::PointerMove_Params tempParams = {
		.widget = *this,
		.rectColl = rectColl,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.customData = params.customData,
		.textEngine = params.textEngine,
		.gizmoTargetSizePx = ViewportGizmo::ComputeTargetSizePx(
			params.window.minimumHeightCm,
			params.window.dpi), };
	auto temp = impl::PointerMove(tempParams);
	return temp.consumed;
}

Gui::TouchEventConsumption InternalViewportWidget::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
	Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto const& rectColl = params.rectCollection;

	impl::PointerMove_Pointer pointer = {
		.pos = params.event.position,
		.posDelta = {},
		.id = params.event.id,
		.occluded = occluded, };

	impl::PointerMove_Params temp = {
		.widget = *this,
		.rectColl = rectColl,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.customData = params.customData,
		.textEngine = params.textEngine,
		.gizmoTargetSizePx = ViewportGizmo::ComputeTargetSizePx(
			params.window.minimumHeightCm,
			params.window.dpi), };
	return impl::PointerMove(temp);
}

Gui::TouchEventConsumption InternalViewportWidget::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	auto const& rectColl = params.rectCollection;

	impl::PointerPress_Pointer pointer = {
		.pos = params.event.position,
		.id = params.event.id,
		.pressed = params.event.pressed,
		.type = impl::PointerType::Primary, };

	impl::PointerPress_Params temp = {
		.customData = params.customData,
		.eventConsumed = consumed,
		.pointer = pointer,
		.rectColl = rectColl,
		.rectCollIter = rectCollIter,
		.textEngine = params.textEngine,
		.transientAlloc = params.transientAlloc,
		.widget = *this,
		.gizmoTargetSizePx = ViewportGizmo::ComputeTargetSizePx(
			params.window.minimumHeightCm,
			params.window.dpi), };
	return impl::PointerPress(temp);
}

ViewportWidget::ViewportWidget(EditorImpl& implData) :
	editorImpl(&implData)
{
	auto* anchorArea = this;

	implData.viewportWidgetPtrs.push_back(this);

	// Inset the joysticks from the screen edges so they don't sit flush in the corner.
	// The magnitude matches the minimum interactable size (default 0.5 cm = 5 mm), the same
	// physical quantity the joystick extent is derived from, and is flagged to scale with
	// content. The AnchorArea resolves it to pixels at layout time using the window DPI.
	constexpr auto joystickEdgeOffset = Gui::Distance::Mm(5.f, true);

	this->leftJoystick = new Joystick;
	Gui::AnchorArea::Node leftJoystickNode = {};
	leftJoystickNode.anchorX = Gui::AnchorArea::AnchorX::Left;
	leftJoystickNode.anchorY = Gui::AnchorArea::AnchorY::Bottom;
	leftJoystickNode.offsetX = joystickEdgeOffset;
	leftJoystickNode.offsetY = joystickEdgeOffset;
	leftJoystickNode.widget = Std::Box{ leftJoystick };
	anchorArea->nodes.push_back(Std::Move(leftJoystickNode));

	this->rightJoystick = new Joystick;
	Gui::AnchorArea::Node rightJoystickNode = {};
	rightJoystickNode.anchorX = Gui::AnchorArea::AnchorX::Right;
	rightJoystickNode.anchorY = Gui::AnchorArea::AnchorY::Bottom;
	rightJoystickNode.offsetX = joystickEdgeOffset;
	rightJoystickNode.offsetY = joystickEdgeOffset;
	rightJoystickNode.widget = Std::Box{ rightJoystick };
	anchorArea->nodes.push_back(Std::Move(rightJoystickNode));

	// Background
	viewport = new InternalViewportWidget(implData);
	anchorArea->backgroundWidget = Std::Box{ viewport };
}

Editor::ViewportWidget::~ViewportWidget()
{
	auto ptrIt = Std::FindIf(
		editorImpl->viewportWidgetPtrs.begin(),
		editorImpl->viewportWidgetPtrs.end(),
		[this](auto const& val) -> bool { return val == this; });
	DENGINE_IMPL_ASSERT(ptrIt != editorImpl->viewportWidgetPtrs.end());
	editorImpl->viewportWidgetPtrs.erase(ptrIt);
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