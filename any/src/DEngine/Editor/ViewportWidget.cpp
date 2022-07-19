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
		DENGINE_IMPL_ASSERT(Math::Abs(rayDirection.Magnitude() - 1.f) <= 0.00001f);
		DENGINE_IMPL_ASSERT(Math::Abs(planeNormal.Magnitude() - 1.f) <= 0.00001f);

		f32 d = Math::Dot(planeNormal, pointOnPlane);

		// Compute the t value for the directed line ab intersecting the plane
		f32 t = (d - Math::Dot(planeNormal, rayOrigin)) / Math::Dot(planeNormal, rayDirection);
		// If t is above 0, the intersection is in front of the ray, not behind.
		if (t >= 0.0f)
			return Std::Opt<f32>{ t };
		else
			return Std::nullOpt;
	}

	struct Rectangle3D
	{
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
		Gizmo::Arrow arrow,
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
		arrow.capDiameter *= gizmoScale;
		arrow.shaftDiameter *= gizmoScale;
		arrow.shaftLength *= gizmoScale;

		auto const localRight = Math::Vec2::Right().GetRotated(gizmoRotation);
		auto const localUp = localRight.GetRotated90(true);
		auto const distToShaftCenter = arrow.shaftLength / 2.f;
		auto const distToCapCenter = arrow.shaftLength + (arrow.capLength / 2.f);

		Std::Opt<f32> closestDistance;
		Gizmo::GizmoPart gizmoPart = {};
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
				gizmoPart = Gizmo::GizmoPart::ArrowX;
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
				gizmoPart = Gizmo::GizmoPart::ArrowX;
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
				gizmoPart = Gizmo::GizmoPart::ArrowY;
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
				gizmoPart = Gizmo::GizmoPart::ArrowY;
			}
		}

		// Then we check the XY quad
		{
			auto planeScale = gizmoScale * Gizmo::defaultPlaneScaleRelative;
			auto planeOffset = gizmoScale * Gizmo::defaultPlaneOffsetRelative;

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
				gizmoPart = Gizmo::GizmoPart::PlaneXY;
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

	[[nodiscard]] static Std::Opt<GizmoHitTest_Translate_ReturnT> GizmoHitTest_Scale(
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
		arrow.capDiameter *= gizmoScale;
		arrow.shaftDiameter *= gizmoScale;
		arrow.shaftLength *= gizmoScale;

		auto const localRight = Math::Vec2::Right().GetRotated(gizmoRotation);
		auto const localUp = localRight.GetRotated90(true);
		auto const distToShaftCenter = arrow.shaftLength / 2.f;
		auto const distToCapCenter = arrow.shaftLength + (arrow.capLength / 2.f);

		Std::Opt<f32> closestDistance;
		Gizmo::GizmoPart gizmoPart = {};
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
				gizmoPart = Gizmo::GizmoPart::ArrowX;
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
				gizmoPart = Gizmo::GizmoPart::ArrowX;
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
				gizmoPart = Gizmo::GizmoPart::ArrowY;
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
				gizmoPart = Gizmo::GizmoPart::ArrowY;
			}
		}

		// Then we check the XY quad
		{
			auto planeScale = gizmoScale * Gizmo::defaultPlaneScaleRelative;
			auto planeOffset = gizmoScale * Gizmo::defaultPlaneOffsetRelative;

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
				gizmoPart = Gizmo::GizmoPart::PlaneXY;
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

	struct GizmoHitTest_ReturnT
	{
		// Holds the hit point relative to the position. No scaling or rotation applied.
		Math::Vec3 relativeHitPoint_Object;
		Gizmo::GizmoPart part;
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
		GizmoType gizmoType)
	{
		Math::Mat4 worldTransform = Math::Mat4::Identity();
		Math::LinAlg3D::SetTranslation(worldTransform, gizmoPosition);

		auto const scale = Gizmo::ComputeScale(
			worldTransform,
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
		Transform& transform)
	{
		auto const rayOrigin = widget.cam.position;
		auto const rayDir = widget.BuildRayDirection(widgetRect, pointerPos);
		
		auto const transformMat = Math::LinAlg3D::Translate(transform.position);
		auto const scale = Gizmo::ComputeScale(
			transformMat,
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
				newPos = gizmoHoldingData.initialPos + movementAxis * Math::Dot(movementAxis, newPos - gizmoHoldingData.initialPos);
			}
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

		auto const hitPoint = rayOrigin + rayDir * distanceOpt.Value();
		auto const hitPointRel = hitPoint - transform.position;

		// We should technically project the hitpoint as 2D vector onto the plane,
		// But the plane is facing directly towards Z anyways so we just ditch the Z component.

		Math::Vec2 temp = hitPointRel.AsVec2().GetNormalized();
		// Calculate angle between this and the world space Up vector.
		transform.rotation = Math::Vec2::SignedAngle(temp, Math::Vec2::Up()) + rotationOffset;
	}

	static void Gizmo_HandleScaling(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		ViewportWidget::HoldingGizmoData const& holdingData,
		Math::Vec2 pointerPos,
		Transform& transform)
	{
		auto const rayOrigin = widget.cam.position;
		auto const rayDir = widget.BuildRayDirection(widgetRect, pointerPos);

		auto transformMat = Math::LinAlg3D::Translate(transform.position);
		auto const scale = Gizmo::ComputeScale(
			transformMat,
			widget.BuildProjectionMatrix(widgetRect.extent.Aspect()),
			widgetRect.extent);
		
		// First raycast against the plane of the object.
		auto pointDist = IntersectRayPlane(rayOrigin, rayDir, Math::Vec3::Forward(), transform.position);
		if (pointDist.HasValue())
		{
			auto const newRelativePoint = (rayOrigin + rayDir * pointDist.Value()) - transform.position;

			switch (holdingData.holdingPart)
			{
				case Gizmo::GizmoPart::ArrowX:
				{
					auto const x = Math::Vec2::Right().GetRotated(transform.rotation).AsVec3();
					auto const dotX = Math::Dot(holdingData.relativeHitPointObject, x) / holdingData.initialObjectScale.x;
					auto const newDotX = Math::Dot(newRelativePoint, x);

					transform.scale.x = newDotX / dotX;
					break;
				}
				case Gizmo::GizmoPart::ArrowY:
				{
					auto const y = Math::Vec2::Up().GetRotated(transform.rotation).AsVec3();
					auto const dotY = Math::Dot(holdingData.relativeHitPointObject, y) / holdingData.initialObjectScale.y;
					auto const newDotY = Math::Dot(newRelativePoint, y);

					transform.scale.y = newDotY / dotY;
					break;
				}
				case Gizmo::GizmoPart::PlaneXY:
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

	struct PointerMove_Pointer {
		Math::Vec2 pos;
		Math::Vec2 posDelta;
		u8 id;
		bool occluded;
	};
	struct PointerMove_Params {
		InternalViewportWidget& widget;
		Gui::Context& ctx;
		Gui::Rect const& widgetRect;
		Gui::Rect const& visibleRect;
		PointerMove_Pointer const& pointer;
	};
	[[nodiscard]] static bool PointerMove(PointerMove_Params const& params) {
		auto& widget = params.widget;
		auto const& widgetRect = params.widgetRect;
		auto const& visibleRect = params.visibleRect;
		auto const& pointer = params.pointer;

		bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);

		if (widget.state == ViewportWidget::BehaviorState::Gizmo)
		{
			DENGINE_IMPL_ASSERT(widget.holdingGizmoData.HasValue());
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
				auto& transform = *transformPtr;
				auto const currGizmo = gizmoHoldingData.gizmoType;
				if (currGizmo == GizmoType::Translate)
				{
					impl::Gizmo_HandleTranslation(
						widget,
						widgetRect,
						gizmoHoldingData,
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
				else if (currGizmo == GizmoType::Scale)
				{
					impl::Gizmo_HandleScaling(
						widget,
						widgetRect,
						gizmoHoldingData,
						pointer.pos,
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

		return pointerInside;
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
		InternalViewportWidget& widget;
		Gui::Context& ctx;
		Gui::Rect const& widgetRect;
		Gui::Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
		bool eventConsumed;
	};
	[[nodiscard]] static bool PointerPress(PointerPress_Params const& params) {
		auto& widget = params.widget;
		auto const& widgetRect = params.widgetRect;
		auto const& visibleRect = params.visibleRect;
		auto const& pointer = params.pointer;
		auto const& oldEventConsumed = params.eventConsumed;

		DENGINE_IMPL_ASSERT(widget.editorImpl);
		auto& editorImpl = *widget.editorImpl;

		bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
		if (!pointerInside && pointer.pressed)
			return false;

		if (widget.holdingGizmoData.HasValue() &&
			widget.holdingGizmoData.Value().pointerId == pointer.id &&
			!pointer.pressed)
		{
			widget.holdingGizmoData = Std::nullOpt;
			widget.state = ViewportWidget::BehaviorState::Normal;
			return false;
		}

		// Initiate free-look
		if (!oldEventConsumed &&
			pointerInside &&
			pointer.pressed &&
			widget.state == ViewportWidget::BehaviorState::Normal &&
			pointer.type == PointerType::Secondary)
		{
			widget.state = ViewportWidget::BehaviorState::FreeLooking;
			editorImpl.appCtx->LockCursor(true);
			return true;
		}

		// Stop free-look
		if (!pointer.pressed &&
			widget.state == ViewportWidget::BehaviorState::FreeLooking &&
			pointer.type == PointerType::Secondary)
		{
			widget.state = ViewportWidget::BehaviorState::Normal;
			editorImpl.appCtx->LockCursor(false);
			return false;
		}


		Scene const& scene = editorImpl.GetActiveScene();

		// We don't want to do gizmo stuff if it was the secondary cursor button
		bool doGizmoStuff =
			!oldEventConsumed &&
			pointerInside &&
			pointer.pressed &&
			widget.state == ViewportWidget::BehaviorState::Normal &&
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
				auto const currGizmoType = editorImpl.GetCurrentGizmoType();
				auto const& hitTestOpt = impl::GizmoHitTest(
					widget,
					widgetRect,
					pointer.pos,
					transform.position,
					transform.rotation,
					currGizmoType);
				if (hitTestOpt.HasValue())
				{
					auto const& hit = hitTestOpt.Value();
					ViewportWidget::HoldingGizmoData holdingGizmoData = {};
					holdingGizmoData.gizmoType = currGizmoType;
					holdingGizmoData.holdingPart = hit.part;
					holdingGizmoData.initialPos = transform.position;
					holdingGizmoData.normalizedOffsetGizmo = hit.normalizedHitPoint_Gizmo;
					holdingGizmoData.pointerId = pointer.id;
					holdingGizmoData.initialObjectScale = transform.scale;
					holdingGizmoData.relativeHitPointObject = hit.relativeHitPoint_Object;
					holdingGizmoData.rotationOffset = hit.rotationOffset;
					widget.holdingGizmoData = holdingGizmoData;
					widget.state = ViewportWidget::BehaviorState::Gizmo;
					hitGizmo = true;
					return pointerInside;
				}
			}
			// If we didn't hit a gizmo, we want to see if we hit any selectable colliders.
			if (!hitGizmo)
			{
				// Collapse this shit into a function
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
						Std::Opt<f32> distanceOpt = Intersect_Ray_PhysicsCollider2D(
							widget,
							{ vertices, 4 },
							transform->position.AsVec2(),
							transform->rotation,
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
				if (hitEntity.HasValue())
				{
					widget.editorImpl->SelectEntity(hitEntity.Value().b);
					return pointerInside;
				}
			}
		}

		return pointerInside;
	}
}

// Target_size in pixels
[[nodiscard]] f32 Gizmo::ComputeScale(
	Math::Mat4 const& worldTransform,
	Math::Mat4 const& projection,
	Gui::Extent viewportSize) noexcept
{
	auto minExtent = Math::Min(viewportSize.width, viewportSize.height);
	auto targetSizePx = (u32)Math::Round((f32)minExtent * Gizmo::defaultGizmoSizeRelative);
	auto const pixelSize = 1.f / (f32)viewportSize.height;
	Math::Vec4 zVec = { worldTransform.At(3, 0), worldTransform.At(3, 1), worldTransform.At(3, 2), worldTransform.At(3, 3) };
	return (f32)targetSizePx * pixelSize * (projection * zVec).w;
}

InternalViewportWidget::InternalViewportWidget(
	EditorImpl& implData) :
	editorImpl(&implData)
{
	auto newViewportRef = editorImpl->gfxCtx->NewViewport();
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

	if (editorImpl->GetSelectedEntity().HasValue())
	{
		Entity selected = editorImpl->GetSelectedEntity().Value();
		// Find Transform component of this entity

		auto transformPtr = scene.GetComponent<Transform>(selected);

		// Draw gizmo
		if (transformPtr != nullptr)
		{
			Transform const& transform = *transformPtr;

			returnVal.gizmoOpt = Gfx::ViewportUpdate::Gizmo{};
			Gfx::ViewportUpdate::Gizmo& gizmo = returnVal.gizmoOpt.Value();
			gizmo.type = impl::ToGfxGizmoType(editorImpl->GetCurrentGizmoType());
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
		auto const rbPtr = scene.GetComponent<Physics::Rigidbody2D>(selected);
		if (rbPtr && transformPtr)
		{
			auto const& transform = *transformPtr;

			Math::Vec2 vertices[4] = {
				{-0.5f, 0.5f },
				{ 0.5f, 0.5f },
				{ 0.5f, -0.5f },
				{ -0.5f, -0.5f } };

			Math::Mat4 worldTransform = Math::Mat4::Identity();
			worldTransform = Math::LinAlg3D::Scale_Homo(transform.scale.AsVec3()) * worldTransform;
			worldTransform = Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, transform.rotation) * worldTransform;
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

Gui::SizeHint InternalViewportWidget::GetSizeHint2(
	Gui::Widget::GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;

	Gui::SizeHint returnVal = {};
	returnVal.minimum = { 450, 450 };
	returnVal.expandX = true;
	returnVal.expandY = true;

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnVal);

	return returnVal;
}

void InternalViewportWidget::Render2(
	Gui::Widget::Render_Params const& params,
	Gui::Rect const& widgetRect,
	Gui::Rect const& visibleRect) const
{
	// First draw the viewport.
	Gfx::GuiDrawCmd drawCmd = {};
	drawCmd.type = Gfx::GuiDrawCmd::Type::Viewport;
	drawCmd.viewport.id = viewportId;
	drawCmd.rectPosition.x = (f32)widgetRect.position.x / params.framebufferExtent.width;
	drawCmd.rectPosition.y = (f32)widgetRect.position.y / params.framebufferExtent.height;
	drawCmd.rectExtent.x = (f32)widgetRect.extent.width / params.framebufferExtent.width;
	drawCmd.rectExtent.y = (f32)widgetRect.extent.height / params.framebufferExtent.height;

	params.drawInfo.drawCmds->push_back(drawCmd);
}

bool InternalViewportWidget::CursorPress2(
	Gui::Widget::CursorPressParams const& params,
	Gui::Rect const& widgetRect,
	Gui::Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.pressed = params.event.pressed;
	pointer.type = impl::ToPointerType(params.event.button);

	impl::PointerPress_Params temp = {
		.widget = *this,
		.ctx = params.ctx,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.eventConsumed = consumed, };

	return impl::PointerPress(temp);
}

bool InternalViewportWidget::CursorMove(
	Gui::Widget::CursorMoveParams const& params,
	Gui::Rect const& widgetRect,
	Gui::Rect const& visibleRect,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };
	pointer.posDelta = { (f32)params.event.positionDelta.x, (f32)params.event.positionDelta.y };
	pointer.occluded = occluded;

	impl::PointerMove_Params temp = {
		.widget = *this,
		.ctx = params.ctx,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer, };
	return impl::PointerMove(temp);
}

bool InternalViewportWidget::TouchMove2(
	TouchMoveParams const& params,
	Gui::Rect const& widgetRect,
	Gui::Rect const& visibleRect,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.posDelta = {};
	pointer.occluded = occluded;

	impl::PointerMove_Params temp = {
		.widget = *this,
		.ctx = params.ctx,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer, };
	return impl::PointerMove(temp);
}

bool InternalViewportWidget::TouchPress2(
	TouchPressParams const& params,
	Gui::Rect const& widgetRect,
	Gui::Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.pressed = params.event.pressed;
	pointer.type = impl::PointerType::Primary;

	impl::PointerPress_Params temp = {
		.widget = *this,
		.ctx = params.ctx,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.eventConsumed = consumed, };
	return impl::PointerPress(temp);
}

ViewportWidget::ViewportWidget(EditorImpl& implData) :
	editorImpl(&implData)
{
	auto* anchorArea = this;

	implData.viewportWidgetPtrs.push_back(this);

	leftJoystick = new Joystick;
	Gui::AnchorArea::Node leftJoystickNode = {};
	leftJoystickNode.anchorX = Gui::AnchorArea::AnchorX::Left;
	leftJoystickNode.anchorY = Gui::AnchorArea::AnchorY::Bottom;
	leftJoystickNode.extent = { 200, 200 };
	leftJoystickNode.widget = Std::Box{ leftJoystick };
	anchorArea->nodes.push_back(Std::Move(leftJoystickNode));

	rightJoystick = new Joystick;
	Gui::AnchorArea::Node rightJoystickNode = {};
	rightJoystickNode.anchorX = Gui::AnchorArea::AnchorX::Right;
	rightJoystickNode.anchorY = Gui::AnchorArea::AnchorY::Bottom;
	rightJoystickNode.extent = { 200, 200 };
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