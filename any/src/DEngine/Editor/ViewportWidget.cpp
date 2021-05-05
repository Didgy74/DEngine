#include "ViewportWidget.hpp"

#include <DEngine/Std/Trait.hpp>

#include <iostream> // For testing

namespace DEngine::Editor::impl
{
	// Translates cursorpos into [-1, 1] of the viewport-rect.
	[[nodiscard]] Math::Vec2 GetNormalizedViewportCoord(
		Math::Vec2 cursorPos,
		Gui::Rect viewportRect) noexcept
	{
		Math::Vec2 cursorNormalizedPos = {
			(cursorPos.x - viewportRect.position.x) / viewportRect.extent.width,
			(cursorPos.y - viewportRect.position.y) / viewportRect.extent.height };
		cursorNormalizedPos -= { 0.5f, 0.5f };
		cursorNormalizedPos *= 2.f;
		cursorNormalizedPos.y = -cursorNormalizedPos.y;
		return cursorNormalizedPos;
	}

	// screenSpacePos in normalized coordinates [-1, 1]
	[[nodiscard]] Math::Vec3 DeprojectScreenspaceCoord(
		Math::Vec2 screenSpacePosNormalized,
		Math::Mat4 projectionMatrix) noexcept
	{
		Math::Vec4 vector = screenSpacePosNormalized.AsVec4(1.f, 1.f);
		vector = projectionMatrix.GetInverse().Value() * vector;
		for (auto& item : vector)
			item /= vector.w;
		return Math::Vec3(vector);
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
		Gizmo::Test_Ray ray,
		Math::Vec3 aIn,
		Math::Vec3 bIn,
		Math::Vec3 cIn)
	{
		// Moller-Trumbore Intersection algorithm

		Math::Vec3 rayOrigin = ray.origin;
		Math::Vec3 rayVector = ray.direction;

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
		if (v < 0.0 || u + v > 1.0)
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
		Gizmo::Test_Ray ray,
		Math::Vec3 cylinderVertA,
		Math::Vec3 cylinderVertB,
		f32 cylinderRadius) noexcept
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
		f32 const k = m.MagnitudeSqrd() - Math::Sqrd(cylinderRadius);
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
			Std::Opt<f32> const hit = IntersectRayPlane(
				ray.origin,
				ray.direction,
				cylinderAxis.GetNormalized(),
				endcap);
			if (hit.HasValue())
			{
				f32 const distance = hit.Value();
				Math::Vec3 const hitPoint = ray.origin + ray.direction * distance;
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
		Gizmo::Test_Ray ray,
		Gizmo::Arrow arrow,
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
	[[nodiscard]] Math::Vec3 TranslateAlongPlane(
		Gizmo::Test_Ray ray,
		Math::Vec3 planeNormal,
		Math::Vec3 initialPosition,
		Gizmo::Test_Ray initialRay) noexcept
	{
		DENGINE_DETAIL_ASSERT(Math::Abs(ray.direction.Magnitude() - 1.f) <= 0.00001f);
		DENGINE_DETAIL_ASSERT(Math::Abs(planeNormal.Magnitude() - 1.f) <= 0.00001f);
		DENGINE_DETAIL_ASSERT(Math::Abs(initialRay.direction.Magnitude() - 1.f) <= 0.00001f);

		// Calculate cursor offset that we'll use to prevent the center of the object from snapping to the cursor
		Math::Vec3 offset = {};
		Std::Opt<f32> hitA = IntersectRayPlane(initialRay.origin, initialRay.direction, planeNormal, initialPosition);
		if (hitA.HasValue())
			offset = initialPosition - (initialRay.origin + initialRay.direction * hitA.Value());

		Std::Opt<f32> hitB = IntersectRayPlane(ray.origin, ray.direction, planeNormal, initialPosition);
		if (hitB.HasValue())
			return ray.origin + ray.direction * hitB.Value() + offset;
		else
			return initialPosition;
	}


	[[nodiscard]] static Std::Opt<Gizmo::GizmoPart> GizmoHitTest_Translate(
		Transform const& transform,
		f32 scale,
		Gizmo::Test_Ray ray) noexcept
	{
		// Gizmo cannot include scale in the world transform, so we modify the arrow struct
		// to account for the scaling
		Gizmo::Arrow arrow = Gizmo::defaultArrow;
		arrow.capLength *= scale;
		arrow.capSize *= scale;
		arrow.shaftDiameter *= scale;
		arrow.shaftLength *= scale;

		Std::Opt<f32> closestDistance;
		Gizmo::GizmoPart gizmoPart = {};
		{
			// Next we handle the X arrow
			Math::Mat4 worldTransform = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::Y, Math::pi / 2);
			Math::LinTran3D::SetTranslation(worldTransform, Math::Vec3{ transform.position.x, transform.position.y, 0.f });

			Std::Opt<f32> hit = IntersectArrow(ray, arrow, worldTransform);
			if (hit.HasValue() && (!closestDistance.HasValue() || hit.Value() < closestDistance.Value()))
			{
				closestDistance = hit.Value();
				gizmoPart = Gizmo::GizmoPart::ArrowX;
			}
		}
		{
			// Next we handle the Y arrow
			Math::Mat4 worldTransform = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::X, -Math::pi / 2);
			Math::LinTran3D::SetTranslation(worldTransform, Math::Vec3{ transform.position.x, transform.position.y, 0.f });

			Std::Opt<f32> hit = IntersectArrow(ray, arrow, worldTransform);
			if (hit.HasValue() && (!closestDistance.HasValue() || hit.Value() < closestDistance.Value()))
			{
				closestDistance = hit.Value();
				gizmoPart = Gizmo::GizmoPart::ArrowY;
			}
		}

		// Then we check the XY quad
		{
			Std::Opt<f32> hit = IntersectRayPlane(ray.origin, ray.direction, Math::Vec3::Forward(), transform.position);
			if (hit.HasValue())
			{
				f32 planeScale = scale * Gizmo::defaultPlaneScaleRelative;
				f32 planeOffset = scale * Gizmo::defaultPlaneOffsetRelative;

				Math::Vec3 quadPosition = transform.position + Math::Vec3{ 1.f, 1.f, 0.f } *planeOffset;

				Math::Vec3 point = ray.origin + ray.direction * hit.Value();
				Math::Vec3 pointRelative = point - quadPosition;
				f32 dotA = Math::Vec3::Dot(pointRelative, Math::Vec3{ 1.f, 0.f, 0.f });
				f32 dotB = Math::Vec3::Dot(pointRelative, Math::Vec3{ 0.f, 1.f, 0.f });
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
			return gizmoPart;
		else
			return Std::nullOpt;
	}

	[[nodiscard]] static Std::Opt<f32> GizmoHitTest_Rotate(
		Transform const& transform,
		f32 scale,
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDirection) noexcept
	{
		auto const hitPointDistOpt = IntersectRayPlane(
			rayOrigin,
			rayDirection,
			Math::Vec3::Forward(),
			transform.position);

		if (!hitPointDistOpt.HasValue())
			return {};

		f32 const distance = hitPointDistOpt.Value();
		Math::Vec3 hitPoint = rayOrigin + (rayDirection * distance);

		f32 const relativeDistSqrd = (hitPoint - transform.position).MagnitudeSqrd();
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

	[[nodiscard]] static Math::Vec3 BuildDeprojectedDirection(
		Gui::Rect widgetRect,
		Math::Vec2 cursorPos,
		Math::Vec3 origin,
		Math::Mat4 projMat)
	{
		Math::Vec2 cursorNormalizedPos = GetNormalizedViewportCoord(cursorPos, widgetRect);
		Math::Vec3 rayDir = (DeprojectScreenspaceCoord(cursorNormalizedPos, projMat) - origin).GetNormalized();
		return rayDir;
	}

	struct GizmoHitTestReturn_T
	{
		Gizmo::GizmoPart part;
		bool hitRotateGizmo;
		f32 rotateOffset;
		Gizmo::Test_Ray ray;
	};

	[[nodiscard]] static Std::Opt<GizmoHitTestReturn_T> GizmoHitTest(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		Math::Vec2 pointerPos,
		Transform const& transform,
		GizmoType gizmoType)
	{
		Math::Mat4 worldTransform = Math::Mat4::Identity();
		Math::LinTran3D::SetTranslation(worldTransform, { transform.position.x, transform.position.y, 0.f });

		f32 const aspectRatio = (f32)widgetRect.extent.width / (f32)widgetRect.extent.height;
		Math::Mat4 projMat = widget.GetProjectionMatrix(aspectRatio);

		u32 smallestViewportExtent = Math::Min(widgetRect.extent.width, widgetRect.extent.height);
		f32 scale = Gizmo::ComputeScale(
			worldTransform,
			u32(smallestViewportExtent * Gizmo::defaultGizmoSizeRelative),
			projMat,
			widgetRect.extent);

		Gizmo::Test_Ray ray = {};
		ray.origin = widget.cam.position;
		ray.direction = BuildDeprojectedDirection(
			widgetRect,
			pointerPos,
			ray.origin,
			projMat);

		if (gizmoType == GizmoType::Translate)
		{
			auto hitOpt = GizmoHitTest_Translate(
				transform,
				scale,
				ray);
			if (hitOpt.HasValue())
			{
				GizmoHitTestReturn_T returnVal = {};
				returnVal.part = hitOpt.Value();
				returnVal.ray = ray;
				return returnVal;
			}
		}
		else if (gizmoType == GizmoType::Rotate)
		{
			auto const hitDistanceOpt = GizmoHitTest_Rotate(
				transform,
				scale,
				ray.origin,
				ray.direction);
			if (hitDistanceOpt.HasValue())
			{
				GizmoHitTestReturn_T returnVal = {};
				returnVal.hitRotateGizmo = true;
				returnVal.ray = ray;
				// Calculate rotation offset from the hitpoint.
				Math::Vec3 hitPoint = ray.origin + ray.direction * hitDistanceOpt.Value();
				Math::Vec2 hitPointRel = (hitPoint - transform.position).AsVec2().GetNormalized();
				Math::Vec2 transformUp = Math::Vec2::Up().GetRotated(transform.rotation);
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
		Gizmo::Test_Ray ray)
	{
		Math::Mat4 transMat = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::Z, rotation);
		Math::LinTran3D::SetTranslation(transMat, position.AsVec3());

		// Compare against all trianges to check if we hit it
		for (uSize i = 1; i < vertices.Size() - 1; i += 1)
		{
			Math::Vec2 tri[3] = { vertices[0], vertices[i], vertices[i + 1] };
			for (uSize j = 0; j < 3; j += 1)
				tri[j] = Math::Vec2(transMat * tri[j].AsVec4(0.f, 1.f));

			Std::Opt<f32> distance = impl::IntersectRayTri(
				ray,
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

		f32 aspectRatio = (f32)widgetRect.extent.width / (f32)widgetRect.extent.height;
		Gizmo::Test_Ray ray = {};
		ray.direction = BuildDeprojectedDirection(
			widgetRect,
			pointerPos,
			transform.position,
			widget.GetProjectionMatrix(aspectRatio));
		ray.origin = widget.cam.position;

		Math::Vec3 newPos = TranslateAlongPlane(
			ray,
			Math::Vec3::Forward(),
			gizmoHoldingData.initialPos,
			gizmoHoldingData.initialRay);
		if (gizmoHoldingData.holdingPart == Gizmo::GizmoPart::ArrowX || gizmoHoldingData.holdingPart == Gizmo::GizmoPart::ArrowY)
		{
			Math::Vec3 movementAxis = {};
			switch (gizmoHoldingData.holdingPart)
			{
			case Gizmo::GizmoPart::ArrowX:
				movementAxis = { 1.f, 0.f, 0.f };
				break;
			case Gizmo::GizmoPart::ArrowY:
				movementAxis = { 0.f, 1.f, 0.f };
				break;
			default:
				DENGINE_DETAIL_UNREACHABLE();
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
		Math::Vec2 cursorPos,
		f32 rotationOffset,
		Transform& transform)
	{
		Math::Vec2 cursorNormalizedPos = GetNormalizedViewportCoord(cursorPos, widgetRect);
		f32 aspectRatio = (f32)widgetRect.extent.width / (f32)widgetRect.extent.height;
		Math::Mat4 projMat = widget.GetProjectionMatrix(aspectRatio);
		Math::Vec3 rayDir = (DeprojectScreenspaceCoord(cursorNormalizedPos, projMat) - widget.cam.position).GetNormalized();

		Gizmo::Test_Ray ray = {};
		ray.origin = widget.cam.position;
		ray.direction = BuildDeprojectedDirection(
			widgetRect,
			cursorPos,
			ray.origin,
			widget.GetProjectionMatrix(aspectRatio));

		// Check if we hit the plane of the gizmo
		auto distanceOpt = IntersectRayPlane(
			ray.origin,
			ray.direction,
			Math::Vec3::Forward(),
			transform.position);

		if (!distanceOpt.HasValue())
			return;

		Math::Vec3 hitPoint = ray.origin + ray.direction * distanceOpt.Value();
		Math::Vec3 hitPointRel = hitPoint - transform.position;

		// We should technically project the hitpoint as 2D vector onto the plane,
		// But the plane is facing directly towards Z anyways so we just ditch the Z component.

		Math::Vec2 temp = hitPointRel.AsVec2().GetNormalized();
		// Calculate angle between this and the world space Up vector.
		transform.rotation = Math::Vec2::SignedAngle(temp, Math::Vec2::Up()) + rotationOffset;
	}

	enum class PointerType : u8
	{
		Primary,
		Secondary
	};
	[[nodiscard]] static PointerType ToPointerType(Gui::CursorButton in) noexcept
	{
		switch (in)
		{
			case Gui::CursorButton::Primary: 
				return PointerType::Primary;
			case Gui::CursorButton::Secondary: 
				return PointerType::Secondary;
			default:
				break;
		}
		DENGINE_DETAIL_UNREACHABLE();
		return {};
	}

	static void InternalViewportWidget_PointerPress(
		InternalViewportWidget& widget,
		Gui::Context& ctx,
		Gui::WindowID windowId,
		Gui::Rect widgetRect,
		Gui::Rect visibleRect,
		Math::Vec2 pointerPos,
		u8 pointerId,
		bool pressed,
		PointerType pointerType,
		Gui::CursorClickEvent* cursorClick,
		Gui::TouchEvent* touchEvent)
	{
		DENGINE_DETAIL_ASSERT(widget.editorImpl);
		auto& editorImpl = *widget.editorImpl;
		Scene const& scene = editorImpl.GetActiveScene();

		bool pointerIsInside = widgetRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);

		// We don't want to do gizmo stuff if it was the secondary cursor button
		if (pointerIsInside && pressed && widget.state == InternalViewportWidget::BehaviorState::Normal &&
				pointerType == PointerType::Primary)
		{
			bool hitGizmo = false;
			// If we have selected an entity, and it has a Transform component,
			// We want to see if we hit it's gizmo.
			if (widget.editorImpl->GetSelectedEntity().HasValue())
			{
				Entity const entity = widget.editorImpl->GetSelectedEntity().Value();
				Transform const* transformPtr = scene.GetComponent<Transform>(entity);
				if (transformPtr)
				{
					Transform const& transform = *transformPtr;
					auto const& hitTestOpt = impl::GizmoHitTest(
						widget,
						widgetRect,
						pointerPos,
						transform,
						editorImpl.GetCurrentGizmoType());
					if (hitTestOpt.HasValue())
					{
						auto const& hit = hitTestOpt.Value();
						InternalViewportWidget::HoldingGizmoData holdingGizmoData = {};
						holdingGizmoData.holdingPart = hit.part;
						holdingGizmoData.initialPos = transform.position;
						holdingGizmoData.initialRay = hit.ray;
						holdingGizmoData.rotationOffset = hit.rotateOffset;
						holdingGizmoData.pointerId = pointerId;
						widget.holdingGizmoData = holdingGizmoData;
						widget.state = InternalViewportWidget::BehaviorState::Gizmo;
						hitGizmo = true;
						return;
					}
				}

				// If we didn't hit a gizmo, we want to see if we hit any selectable colliders.
				if (!hitGizmo)
				{
					Std::Opt<Std::Pair<f32, Entity>> hitEntity;
					Gizmo::Test_Ray ray = widget.BuildRay(widgetRect, pointerPos);
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
								ray);
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
						return;
					}
				}
			}
		}

		if (widget.state == InternalViewportWidget::BehaviorState::Gizmo && !pressed)
		{
			DENGINE_DETAIL_ASSERT(widget.holdingGizmoData.HasValue());
			widget.holdingGizmoData = Std::nullOpt;
			widget.state = InternalViewportWidget::BehaviorState::Normal;
			return;
		}

		// Initiate free-look
		if (pointerIsInside && pressed && widget.state == InternalViewportWidget::BehaviorState::Normal &&
			cursorClick && cursorClick->button == Gui::CursorButton::Secondary)
		{
			widget.state = InternalViewportWidget::BehaviorState::FreeLooking;
			App::LockCursor(true);
			return;
		}

		// Stop free-look
		if (!pressed && widget.state == InternalViewportWidget::BehaviorState::FreeLooking &&
			cursorClick && cursorClick->button == Gui::CursorButton::Secondary)
		{
			widget.state = InternalViewportWidget::BehaviorState::Normal;
			App::LockCursor(false);
			return;
		}
		
	}

	static void InternalViewportWidget_PointerMove(
		InternalViewportWidget& widget,
		Gui::Context& ctx,
		Gui::WindowID windowId,
		Gui::Rect widgetRect,
		Gui::Rect visibleRect,
		Math::Vec2 pointerPos,
		u8 pointerId,
		Gui::CursorMoveEvent* cursorMove,
		Gui::TouchEvent* touchEvent)
	{
		if (widget.state == InternalViewportWidget::BehaviorState::Gizmo)
		{
			DENGINE_DETAIL_ASSERT(widget.holdingGizmoData.HasValue());
			auto const& gizmoHoldingData = widget.holdingGizmoData.Value();
			if (gizmoHoldingData.pointerId == pointerId && widget.editorImpl->GetSelectedEntity().HasValue())
			{
				Entity entity = widget.editorImpl->GetSelectedEntity().Value();
				// First check if we have a transform
				Scene& scene = widget.editorImpl->GetActiveScene();
				Transform* transformPtr = scene.GetComponent<Transform>(entity);
				if (transformPtr != nullptr)
				{
					Transform& transform = *transformPtr;

					GizmoType const currGizmo = widget.editorImpl->GetCurrentGizmoType();
					if (currGizmo == GizmoType::Translate)
					{
						impl::TranslateAlongGizmoAxis(
							widget,
							widgetRect,
							pointerPos,
							transform);
					}
					else if (currGizmo == GizmoType::Rotate)
					{
						impl::Gizmo_HandleRotation(
							widget,
							widgetRect,
							pointerPos,
							gizmoHoldingData.rotationOffset,
							transform);
					}
				}
			}
		}

		if (widget.state == InternalViewportWidget::BehaviorState::FreeLooking)
		{
			DENGINE_DETAIL_ASSERT(cursorMove);
			f32 sensitivity = 0.2f;
			Math::Vec2 amount = { (f32)cursorMove->positionDelta.x, (f32)-cursorMove->positionDelta.y };
			widget.ApplyCameraRotation(amount * sensitivity * Math::degToRad);
		}
	}
}

using namespace DEngine;
using namespace DEngine::Editor;

[[nodiscard]] Gizmo::Test_Ray InternalViewportWidget::BuildRay(Gui::Rect widgetRect, Math::Vec2 pointerPos) const
{
	f32 aspectRatio = (f32)widgetRect.extent.width / (f32)widgetRect.extent.height;
	Math::Mat4 projMat = GetProjectionMatrix(aspectRatio);

	Gizmo::Test_Ray ray{};
	ray.origin = cam.position;
	Math::Vec3 vector = impl::DeprojectScreenspaceCoord(impl::GetNormalizedViewportCoord(pointerPos, widgetRect), projMat);
	ray.direction = (vector - cam.position).GetNormalized();
	return ray;
}

// target_size in pixels
[[nodiscard]] f32 Gizmo::ComputeScale(
	Math::Mat4 const& worldTransform,
	u32 targetSizePx,
	Math::Mat4 const& projection,
	Gui::Extent viewportSize) noexcept
{
	f32 const pixelSize = 1.f / viewportSize.height;
	Math::Vec4 zVec = { worldTransform.At(3, 0), worldTransform.At(3, 1), worldTransform.At(3, 2), worldTransform.At(3, 3) };
	return targetSizePx * pixelSize * (projection * zVec).w;
}

InternalViewportWidget::InternalViewportWidget(
	EditorImpl& implData, 
	Gfx::Context& gfxCtxIn) 
	: gfxCtx(&gfxCtxIn), editorImpl(&implData)
{
	implData.viewportWidgets.push_back(this);

	auto newViewportRef = gfxCtx->NewViewport();
	viewportId = newViewportRef.ViewportID();
}

InternalViewportWidget::~InternalViewportWidget()
{
	gfxCtx->DeleteViewport(viewportId);
	auto ptrIt = Std::FindIf(
		editorImpl->viewportWidgets.begin(),
		editorImpl->viewportWidgets.end(),
		[this](decltype(editorImpl->viewportWidgets)::value_type const& val) -> bool { return val == this; });
	DENGINE_DETAIL_ASSERT(ptrIt != editorImpl->viewportWidgets.end());
	editorImpl->viewportWidgets.erase(ptrIt);
}

Math::Mat4 InternalViewportWidget::GetViewMatrix() const noexcept
{
	Math::Mat4 camMat = Math::LinTran3D::Rotate_Homo(cam.rotation);
	Math::LinTran3D::SetTranslation(camMat, cam.position);
	return camMat;
}

Math::Mat4 InternalViewportWidget::GetPerspectiveMatrix(f32 aspectRatio) const noexcept
{
	return Math::LinTran3D::Perspective_RH_ZO(
		cam.verticalFov * Math::degToRad, 
		aspectRatio, 
		0.1f,
		100.f);
}

Math::Mat4 InternalViewportWidget::GetProjectionMatrix(f32 aspectRatio) const noexcept
{
	Math::Mat4 camMat = Math::Mat4::Identity();
	camMat.At(0, 0) = -1;
	//test.At(1, 1) = -1;
	camMat.At(2, 2) = -1;
	camMat = GetViewMatrix() * camMat;
	camMat = camMat.GetInverse().Value();
	return GetPerspectiveMatrix(aspectRatio) * camMat;
}

void InternalViewportWidget::ApplyCameraRotation(Math::Vec2 input) noexcept
{
	cam.rotation = Math::UnitQuat::FromVector(Math::Vec3::Up(), -input.x) * cam.rotation;
	// Limit rotation up and down
	Math::Vec3 forward = Math::LinTran3D::ForwardVector(cam.rotation);
	f32 dot = Math::Vec3::Dot(forward, Math::Vec3::Up());
	constexpr f32 upDownDotProductLimit = 0.9f;
	if ((dot <= -upDownDotProductLimit && input.y < 0) || (dot >= upDownDotProductLimit && input.y > 0))
		input.y = 0;
	cam.rotation = Math::UnitQuat::FromVector(Math::LinTran3D::RightVector(cam.rotation), -input.y) * cam.rotation;
}

void InternalViewportWidget::ApplyCameraMovement(Math::Vec3 move, f32 speed) noexcept
{
	if (move.MagnitudeSqrd() > 0.f)
	{
		if (move.MagnitudeSqrd() > 1.f)
			move.Normalize();
		Math::Vec3 moveVector{};
		moveVector += Math::LinTran3D::ForwardVector(cam.rotation) * move.z;
		moveVector += Math::LinTran3D::RightVector(cam.rotation) * -move.x;
		moveVector += Math::Vec3::Up() * move.y;

		if (moveVector.MagnitudeSqrd() > 1.f)
			moveVector.Normalize();
		cam.position += moveVector * speed;
	}
}

Math::Vec2 InternalViewportWidget::GetLeftJoystickVec() const noexcept
{
	auto& joystick = joysticks[0];
	if (joystick.isClicked || joystick.touchID.HasValue())
	{
		// Find vector between circle start position, and current position
		// Then apply the logic
		Math::Vec2Int relativeVector = joystick.currentPosition - joystick.originPosition;
		if (relativeVector.MagnitudeSqrd() <= Math::Sqrd(joystickPixelDeadZone))
			return {};
		Math::Vec2 relativeVectorFloat = { (f32)relativeVector.x, (f32)relativeVector.y };
		relativeVectorFloat.x /= joystickPixelRadius;
		relativeVectorFloat.y /= joystickPixelRadius;

		return relativeVectorFloat;
	}
	return {};
}

Math::Vec2 InternalViewportWidget::GetRightJoystickVec() const noexcept
{
	auto& joystick = joysticks[1];
	if (joystick.isClicked || joystick.touchID.HasValue())
	{
		// Find vector between circle start position, and current position
		// Then apply the logic
		Math::Vec2Int relativeVector = joystick.currentPosition - joystick.originPosition;
		if (relativeVector.MagnitudeSqrd() <= Math::Sqrd(joystickPixelDeadZone))
			return {};
		Math::Vec2 relativeVectorFloat = { (f32)relativeVector.x, (f32)relativeVector.y };
		relativeVectorFloat.x /= joystickPixelRadius;
		relativeVectorFloat.y /= joystickPixelRadius;

		return relativeVectorFloat;
	}
	return {};
}

void InternalViewportWidget::TickTest(f32 deltaTime) noexcept
{
	if (!currentlyResizing && currentExtent != newExtent)
	{
		extentCorrectTickCounter += 1;
		if (extentCorrectTickCounter >= 30)
			currentExtent = newExtent;
	}
	if (currentlyResizing)
		extentCorrectTickCounter = 0;

	// Handle camera movement
	if (state == BehaviorState::FreeLooking)
	{
		f32 moveSpeed = 5.f;
		Math::Vec3 moveVector{};
		if (App::ButtonValue(App::Button::W))
			moveVector.z += 1;
		if (App::ButtonValue(App::Button::S))
			moveVector.z -= 1;
		if (App::ButtonValue(App::Button::D))
			moveVector.x += 1;
		if (App::ButtonValue(App::Button::A))
			moveVector.x -= 1;
		if (App::ButtonValue(App::Button::Space))
			moveVector.y += 1;
		if (App::ButtonValue(App::Button::LeftCtrl))
			moveVector.y -= 1;
		ApplyCameraMovement(moveVector, moveSpeed * deltaTime);
	}

	for (uSize i = 0; i < 2; i++)
	{
		auto& joystick = joysticks[i];
		if (joystick.isClicked || joystick.touchID.HasValue())
		{
			// Find vector between circle start position, and current position
			// Then apply the logic
			Math::Vec2Int relativeVector = joystick.currentPosition - joystick.originPosition;
			if (relativeVector.MagnitudeSqrd() <= Math::Sqrd(joystickPixelDeadZone))
				continue;
			Math::Vec2 relativeVectorFloat = { (f32)relativeVector.x, (f32)relativeVector.y };
			relativeVectorFloat.x /= joystickPixelRadius;
			relativeVectorFloat.y /= joystickPixelRadius;

			if (i == 0)
			{
				f32 moveSpeed = 5.f;
				ApplyCameraMovement({ relativeVectorFloat.x, 0.f, -relativeVectorFloat.y }, moveSpeed * deltaTime);
			}
			else
			{
				f32 sensitivity = 1.5f;
				ApplyCameraRotation(Math::Vec2{ relativeVectorFloat.x, -relativeVectorFloat.y } *sensitivity * deltaTime);
			}
		}
		else
			joystick.currentPosition = joystick.originPosition;
	}
}

// Pixel space
void InternalViewportWidget::UpdateJoystickOrigin(Gui::Rect widgetRect) const noexcept
{
	joysticks[0].originPosition.x = widgetRect.position.x + joystickPixelRadius * 2;
	joysticks[0].originPosition.y = widgetRect.position.y + widgetRect.extent.height - joystickPixelRadius * 2;

	joysticks[1].originPosition.x = widgetRect.position.x + widgetRect.extent.width - joystickPixelRadius * 2;
	joysticks[1].originPosition.y = widgetRect.position.y + widgetRect.extent.height - joystickPixelRadius * 2;
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
		DENGINE_DETAIL_UNREACHABLE();
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
	Math::Mat4 projMat = GetProjectionMatrix(aspectRatio);

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
			Math::LinTran3D::SetTranslation(worldTransform, { transform.position.x, transform.position.y, 0.f });
			u32 smallestViewportExtent = Math::Min(newExtent.width, newExtent.height);
			f32 scale = Gizmo::ComputeScale(
				worldTransform,
				u32(smallestViewportExtent * Gizmo::defaultGizmoSizeRelative),
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
			worldTransform = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::Z, transform.rotation) * worldTransform;
			Math::LinTran3D::SetTranslation(worldTransform, transform.position);

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

void InternalViewportWidget::CursorClick(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Math::Vec2Int cursorPos,
	Gui::CursorClickEvent event)
{
	impl::InternalViewportWidget_PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		{ (f32)cursorPos.x, (f32)cursorPos.y },
		InternalViewportWidget::cursorPointerId,
		event.clicked,
		impl::ToPointerType(event.button),
		&event,
		nullptr);
}

void InternalViewportWidget::CursorMove(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Gui::CursorMoveEvent event)
{
	impl::InternalViewportWidget_PointerMove(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		{ (f32)event.position.x, (f32)event.position.y },
		cursorPointerId,
		&event,
		nullptr);
}

void InternalViewportWidget::TouchEvent(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Gui::TouchEvent touch)
{
	if (touch.type == Gui::TouchEventType::Down || touch.type == Gui::TouchEventType::Up)
	{
		bool isPressed = touch.type == Gui::TouchEventType::Down;
		impl::InternalViewportWidget_PointerPress(
			*this,
			ctx,
			windowId,
			widgetRect,
			visibleRect,
			touch.position,
			touch.id,
			isPressed,
			impl::PointerType::Primary,
			nullptr,
			&touch);
	}

	if (touch.type == Gui::TouchEventType::Moved)
	{
		impl::InternalViewportWidget_PointerMove(
			*this,
			ctx,
			windowId,
			widgetRect,
			visibleRect,
			touch.position,
			touch.id,
			nullptr,
			&touch);
	}
}

Gui::SizeHint InternalViewportWidget::GetSizeHint(Gui::Context const& ctx) const
{
	Gui::SizeHint returnVal{};
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
	UpdateJoystickOrigin(widgetRect);
	isVisible = true;
	currentlyResizing = newExtent != widgetRect.extent;
	newExtent = widgetRect.extent;

	// First draw the viewport.
	Gfx::GuiDrawCmd drawCmd{};
	drawCmd.type = Gfx::GuiDrawCmd::Type::Viewport;
	drawCmd.viewport.id = viewportId;
	drawCmd.rectPosition.x = (f32)widgetRect.position.x / framebufferExtent.width;
	drawCmd.rectPosition.y = (f32)widgetRect.position.y / framebufferExtent.height;
	drawCmd.rectExtent.x = (f32)widgetRect.extent.width / framebufferExtent.width;
	drawCmd.rectExtent.y = (f32)widgetRect.extent.height / framebufferExtent.height;
	drawInfo.drawCmds->push_back(drawCmd);

	// Draw a circle, start from the top, move clockwise
	Gfx::GuiDrawCmd::MeshSpan circleMeshSpan{};
	{
		u32 circleVertexCount = 30;
		circleMeshSpan.vertexOffset = (u32)drawInfo.vertices->size();
		circleMeshSpan.indexOffset = (u32)drawInfo.indices->size();
		circleMeshSpan.indexCount = circleVertexCount * 3;
		// Create the vertices, we insert the middle vertex first.
		drawInfo.vertices->push_back({});
		for (u32 i = 0; i < circleVertexCount; i++)
		{
			Gfx::GuiVertex newVertex{};
			f32 currentRadians = 2 * Math::pi / circleVertexCount * i;
			newVertex.position.x += Math::Sin(currentRadians);
			newVertex.position.y += Math::Cos(currentRadians);
			drawInfo.vertices->push_back(newVertex);
		}
		// Build indices
		for (u32 i = 0; i < circleVertexCount - 1; i++)
		{
			drawInfo.indices->push_back(i + 1);
			drawInfo.indices->push_back(0);
			drawInfo.indices->push_back(i + 2);
		}
		drawInfo.indices->push_back(circleVertexCount);
		drawInfo.indices->push_back(0);
		drawInfo.indices->push_back(1);
	}
	// Draw both joysticks using said circle mesh
	for (auto const& joystick : joysticks)
	{
		{
			Gfx::GuiDrawCmd cmd{};
			cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
			cmd.filledMesh.color = { 0.f, 0.f, 0.f, 0.25f };
			cmd.filledMesh.mesh = circleMeshSpan;
			cmd.rectPosition.x = (f32)joystick.originPosition.x / framebufferExtent.width;
			cmd.rectPosition.y = (f32)joystick.originPosition.y / framebufferExtent.height;
			cmd.rectExtent.x = (f32)joystickPixelRadius * 2 / framebufferExtent.width;
			cmd.rectExtent.y = (f32)joystickPixelRadius * 2 / framebufferExtent.height;
			drawInfo.drawCmds->push_back(cmd);
		}

		Gfx::GuiDrawCmd cmd{};
		cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
		cmd.filledMesh.color = { 1.f, 1.f, 1.f, 0.75f };
		cmd.filledMesh.mesh = circleMeshSpan;
		cmd.rectPosition.x = (f32)joystick.currentPosition.x / framebufferExtent.width;
		cmd.rectPosition.y = (f32)joystick.currentPosition.y / framebufferExtent.height;
		cmd.rectExtent.x = (f32)joystickPixelRadius / framebufferExtent.width;
		cmd.rectExtent.y = (f32)joystickPixelRadius / framebufferExtent.height;
		drawInfo.drawCmds->push_back(cmd);
	}
}

ViewportWidget::ViewportWidget(EditorImpl& implData, Gfx::Context& ctx) :
	Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
{
	// Generate top navigation bar
	Gui::Button* settingsBtn = new Gui::Button;
	settingsBtn->text = "Settings";
	AddWidget(Std::Box<Gui::Widget>{ settingsBtn });

	InternalViewportWidget* viewport = new InternalViewportWidget(implData, ctx);
	AddWidget(Std::Box<Gui::Widget>{ viewport });
}