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
		Gizmo::Test_Ray ray,
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
				ray,
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
		Math::Vec3 offset{};
		Std::Opt<f32> hitA = IntersectRayPlane(initialRay, planeNormal, initialPosition);
		if (hitA.HasValue())
			offset = initialPosition - (initialRay.origin + initialRay.direction * hitA.Value());

		Std::Opt<f32> hitB = IntersectRayPlane(ray, planeNormal, initialPosition);
		if (hitB.HasValue())
			return ray.origin + ray.direction * hitB.Value() + offset;
		else
			return initialPosition;
	}

	struct GizmoHitTestReturnType
	{
		Gizmo::GizmoPart part{};
		Gizmo::Test_Ray ray{};
	};

	static Std::Opt<GizmoHitTestReturnType> GizmoHitTest(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		Math::Vec2 cursorPos,
		Transform const& transform)
	{
		Math::Mat4 worldTransform = Math::Mat4::Identity();
		Math::LinTran3D::SetTranslation(worldTransform, { transform.position.x, transform.position.y, 0.f });

		f32 aspectRatio = (f32)widgetRect.extent.width / (f32)widgetRect.extent.height;
		Math::Mat4 projMat = widget.GetProjectionMatrix(aspectRatio);

		u32 smallestViewportExtent = Math::Min(widgetRect.extent.width, widgetRect.extent.height);
		f32 scale = Gizmo::ComputeScale(
			worldTransform,
			u32(smallestViewportExtent * Gizmo::defaultGizmoSizeRelative),
			projMat,
			widgetRect.extent);

		// Gizmo cannot include scale in the world transform, so we modify the arrow struct
		// to account for the scaling
		Gizmo::Arrow arrow = Gizmo::defaultArrow;
		arrow.capLength *= scale;
		arrow.capSize *= scale;
		arrow.shaftDiameter *= scale;
		arrow.shaftLength *= scale;

		Gizmo::Test_Ray ray{};
		ray.origin = widget.cam.position;
		Math::Vec3 vector = DeprojectScreenspaceCoord(GetNormalizedViewportCoord(cursorPos, widgetRect), projMat);
		ray.direction = (vector - widget.cam.position).GetNormalized();

		Std::Opt<GizmoHitTestReturnType> returnVal{};

		bool gizmoPartHit = false;
		f32 closestDistance = 0.f;
		Gizmo::GizmoPart gizmoPart{};
		{
			// Next we handle the X arrow
			Math::Mat4 worldTransform = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::Y, Math::pi / 2);
			Math::LinTran3D::SetTranslation(worldTransform, Math::Vec3{ transform.position.x, transform.position.y, 0.f });

			Std::Opt<f32> hit = IntersectArrow(ray, arrow, worldTransform);
			if (hit.HasValue() && (!gizmoPartHit || hit.Value() < closestDistance))
			{
				closestDistance = hit.Value();
				gizmoPartHit = true;
				gizmoPart = Gizmo::GizmoPart::ArrowX;
			}
		}
		{
			// Next we handle the Y arrow
			Math::Mat4 worldTransform = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::X, -Math::pi / 2);
			Math::LinTran3D::SetTranslation(worldTransform, Math::Vec3{ transform.position.x, transform.position.y, 0.f });

			Std::Opt<f32> hit = IntersectArrow(ray, arrow, worldTransform);
			if (hit.HasValue() && (!gizmoPartHit || hit.Value() < closestDistance))
			{
				closestDistance = hit.Value();
				gizmoPartHit = true;
				gizmoPart = Gizmo::GizmoPart::ArrowY;
			}
		}

		// Then we check the XY quad
		{
			Std::Opt<f32> hit = IntersectRayPlane(ray, Math::Vec3{ 0.f, 0.f, 1.f }, transform.position);
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
					if (!gizmoPartHit || hit.Value() < closestDistance)
					{
						closestDistance = hit.Value();
						gizmoPartHit = true;
						gizmoPart = Gizmo::GizmoPart::PlaneXY;
					}
				}
			}
		}

		if (gizmoPartHit)
		{
			GizmoHitTestReturnType returnVal;
			returnVal.part = gizmoPart;
			returnVal.ray = ray;
			return Std::Opt<GizmoHitTestReturnType>{ returnVal };
		}
		else
			return Std::nullOpt;
	}

	static bool Intersect_Ray_PhysicsCollider(
		InternalViewportWidget& widget,
		b2Body const* phBody,
		Editor::Gizmo::Test_Ray ray)
	{
		b2Shape const* phBaseShape = phBody->GetFixtureList()->GetShape();
		DENGINE_DETAIL_ASSERT(phBaseShape->GetType() == b2Shape::Type::e_polygon);
		b2PolygonShape const* phShape = static_cast<b2PolygonShape const*>(phBaseShape);
		Std::Span<b2Vec2 const> vertices = { phShape->m_vertices, (uSize)phShape->m_count };

		b2Vec2 const position = phBody->GetPosition();
		f32 const rotation = phBody->GetAngle();
		Math::Mat4 transMat = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::Z, rotation);
		Math::LinTran3D::SetTranslation(transMat, { position.x, position.y, 0.f });

		// Compare against all trianges to check if we hit it
		for (uSize i = 1; i < vertices.Size() - 1; i += 1)
		{
			Math::Vec3 tri[3] = {
				{ vertices[0].x, vertices[0].y, 0.f },
				{ vertices[i].x, vertices[i].y, 0.f },
				{ vertices[i + 1].x, vertices[i + 1].y, 0.f } };
			for (uSize j = 0; j < 3; j += 1)
				tri[j] = Math::Vec3(transMat * tri[j].AsVec4(1.f));

			Std::Opt<f32> distance = impl::IntersectRayTri(ray, tri[0], tri[1], tri[2]);
			if (distance.HasValue())
				return true;
		}

		return false;
	}

	static void TranslateAlongGizmoAxis(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		Math::Vec2 cursorPos,
		Transform& transform)
	{
		Math::Vec2 cursorNormalizedPos = GetNormalizedViewportCoord(cursorPos, widgetRect);
		f32 aspectRatio = (f32)widgetRect.extent.width / (f32)widgetRect.extent.height;
		Math::Mat4 projMat = widget.GetProjectionMatrix(aspectRatio);
		Math::Vec3 rayDir = (DeprojectScreenspaceCoord(cursorNormalizedPos, projMat) - widget.cam.position).GetNormalized();

		Gizmo::Test_Ray ray{};
		ray.direction = rayDir;
		ray.origin = widget.cam.position;

		Math::Vec3 newPos = TranslateAlongPlane(ray, Math::Vec3::Forward(), widget.gizmo_initialPos, widget.gizmo_initialRay);
		if (widget.gizmo_holdingPart == Gizmo::GizmoPart::ArrowX || widget.gizmo_holdingPart == Gizmo::GizmoPart::ArrowY)
		{
			Math::Vec3 movementAxis;
			switch (widget.gizmo_holdingPart)
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
			newPos = widget.gizmo_initialPos + movementAxis * Math::Vec3::Dot(movementAxis, newPos - widget.gizmo_initialPos);
		}
		else
		{

		}

		transform.position = newPos;
	}

	static void InternalViewportWidget_HandlePointerPress(
		InternalViewportWidget& widget,
		Gui::Rect widgetRect,
		Math::Vec2 cursorPos,
		Std::Opt<u8> touchID)
	{
		DENGINE_DETAIL_ASSERT(!widget.isCurrentlyHoldingGizmo);
		DENGINE_DETAIL_ASSERT(widgetRect.PointIsInside(cursorPos));

		f32 aspectRatio = (f32)widgetRect.extent.width / (f32)widgetRect.extent.height;
		Math::Mat4 projMat = widget.GetProjectionMatrix(aspectRatio);
		Math::Vec3 deprojCursor = impl::DeprojectScreenspaceCoord(impl::GetNormalizedViewportCoord(cursorPos, widgetRect), projMat);
		Gizmo::Test_Ray ray{};
		ray.origin = widget.cam.position;
		ray.direction = (deprojCursor - widget.cam.position).GetNormalized();

		// Check if we hit the gizmo
		if (widget.editorImpl->selectedEntity.HasValue())
		{
			Entity entity = widget.editorImpl->selectedEntity.Value();
			// First check if we have a transform
			Transform const* transformPtr = widget.editorImpl->scene->GetComponent<Transform>(entity);
			if (transformPtr != nullptr)
			{
				Transform const& transform = *transformPtr;

				auto hitResult = impl::GizmoHitTest(
					widget,
					widgetRect,
					cursorPos,
					transform);

				if (hitResult.HasValue())
				{
					auto const& hit = hitResult.Value();
					widget.isCurrentlyHoldingGizmo = true;
					widget.gizmo_initialRay = hit.ray;
					widget.gizmo_initialPos = transform.position;
					widget.gizmo_holdingPart = hit.part;
					widget.gizmo_touchID = touchID;
				}
			}
		}

		// If we didn't hit the gizmo,
		// Check if we hit a collider
		if (!widget.isCurrentlyHoldingGizmo)
		{
			bool hitSelectedEntity = false;
			if (widget.editorImpl->selectedEntity.HasValue())
			{
				Entity entity = widget.editorImpl->selectedEntity.Value();
				Transform const* transform = widget.editorImpl->scene->GetComponent<Transform>(entity);
				Box2D_Component const* phBodyComp = widget.editorImpl->scene->GetComponent<Box2D_Component>(entity);
				if (transform && phBodyComp)
				{
					bool hit = Intersect_Ray_PhysicsCollider(
						widget,
						phBodyComp->ptr,
						ray);
					if (hit)
					{
						hitSelectedEntity = true;
					}
				}
			}
			if (!hitSelectedEntity)
			{
				// Iterate over all physics components that also have a transform component
				for (auto const& pBodyComponentPair : widget.editorImpl->scene->GetAllComponents<Box2D_Component>())
				{
					Entity entity = pBodyComponentPair.a;

					if (widget.editorImpl->selectedEntity.HasValue() && widget.editorImpl->selectedEntity.Value() == entity)
						continue;

					Transform const* transform = widget.editorImpl->scene->GetComponent<Transform>(entity);
					if (transform)
					{
						bool hit = Intersect_Ray_PhysicsCollider(
							widget,
							pBodyComponentPair.b.ptr,
							ray);
						if (hit)
						{
							widget.editorImpl->SelectEntity(entity);
							break;
						}
					}
				}
			}
		}
	}
}

using namespace DEngine;
using namespace DEngine::Editor;

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
	this->isVisible = false;

	// Handle camera movement
	if (isCurrentlyClicked)
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

void InternalViewportWidget::CursorClick(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Math::Vec2Int cursorPos,
	Gui::CursorClickEvent event)
{
	UpdateJoystickOrigin(widgetRect);
	bool cursorIsInside = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);

	if (isCurrentlyHoldingGizmo && event.button == Gui::CursorButton::Primary && !event.clicked)
	{
		isCurrentlyHoldingGizmo = false;
	}

	if (!isCurrentlyHoldingGizmo && event.clicked && event.button == Gui::CursorButton::Primary && cursorIsInside)
	{
		impl::InternalViewportWidget_HandlePointerPress(
			*this,
			widgetRect,
			Math::Vec2{ (f32)cursorPos.x, (f32)cursorPos.y },
			Std::nullOpt);
	}

	if (event.button == Gui::CursorButton::Secondary && !event.clicked && isCurrentlyClicked)
	{
		isCurrentlyClicked = false;
		App::LockCursor(false);
	}

	if (event.button == Gui::CursorButton::Secondary && event.clicked && !isCurrentlyClicked && cursorIsInside)
	{
		isCurrentlyClicked = true;
		App::LockCursor(true);
	}

	if (event.button == Gui::CursorButton::Primary && event.clicked)
	{
		for (auto& joystick : joysticks)
		{
			Math::Vec2Int relativeVector = cursorPos - joystick.originPosition;
			if (relativeVector.MagnitudeSqrd() <= Math::Sqrd(joystickPixelRadius))
			{
				// Cursor is inside circle
				joystick.isClicked = true;
				joystick.currentPosition = cursorPos;
			}
		}
	}

	if (event.button == Gui::CursorButton::Primary && !event.clicked)
	{
		for (auto& joystick : joysticks)
		{
			joystick.isClicked = false;
			joystick.currentPosition = joystick.originPosition;
		}
	}
}

void InternalViewportWidget::CursorMove(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Gui::CursorMoveEvent event)
{
	UpdateJoystickOrigin(widgetRect);

	if (isCurrentlyHoldingGizmo)
	{
		if (this->editorImpl->selectedEntity.HasValue())
		{
			Entity entity = editorImpl->selectedEntity.Value();
			// First check if we have a transform
			Transform* transformPtr = editorImpl->scene->GetComponent<Transform>(entity);
			if (transformPtr != nullptr)
			{
				Transform& transform = *transformPtr;

				impl::TranslateAlongGizmoAxis(
					*this,
					widgetRect,
					Math::Vec2{ (f32)event.position.x, (f32)event.position.y },
					transform);
			}
		}
	}

	// Handle regular kb+mouse style camera movement
	if (isCurrentlyClicked)
	{
		f32 sensitivity = 0.2f;
		Math::Vec2 amount = { (f32)event.positionDelta.x, (f32)-event.positionDelta.y };
		ApplyCameraRotation(amount * sensitivity * Math::degToRad);
	}

	for (auto& joystick : joysticks)
	{
		if (joystick.isClicked)
		{
			joystick.currentPosition = event.position;
			Math::Vec2Int relativeVector = joystick.currentPosition - joystick.originPosition;
			Math::Vec2 relativeVectorFloat = { (f32)relativeVector.x, (f32)relativeVector.y };
			if (relativeVectorFloat.MagnitudeSqrd() >= Math::Sqrd(joystickPixelRadius))
			{
				relativeVectorFloat = relativeVectorFloat.GetNormalized() * (f32)joystickPixelRadius;
				joystick.currentPosition.x = joystick.originPosition.x + (i32)Math::Round(relativeVectorFloat.x);
				joystick.currentPosition.y = joystick.originPosition.y + (i32)Math::Round(relativeVectorFloat.y);
			}
		}
	}
}

void InternalViewportWidget::TouchEvent(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Gui::TouchEvent touch)
{
	UpdateJoystickOrigin(widgetRect);

	bool touchIsInside = widgetRect.PointIsInside(touch.position) && visibleRect.PointIsInside(touch.position);

	if (touch.type == Gui::TouchEventType::Down && touchIsInside)
	{
		bool hitJoystick = false;
		for (auto& joystick : joysticks)
		{
			if (hitJoystick)
				break;
			// If this joystick already uses touch-point, we go to the next joystick
			if (joystick.touchID.HasValue())
				continue;
			Math::Vec2Int fingerPos = { (i32)touch.position.x, (i32)touch.position.y };
			Math::Vec2Int relativeVector = fingerPos - joystick.originPosition;
			if (relativeVector.MagnitudeSqrd() <= Math::Sqrd(joystickPixelRadius))
			{
				// Cursor is inside circle
				joystick.touchID = touch.id;
				joystick.currentPosition = fingerPos;
				hitJoystick = true;
			}
		}

		// We didn't hit any of the joysticks, check if we hit any of the gizmo arrows.
		if (!hitJoystick && !isCurrentlyHoldingGizmo)
		{
			impl::InternalViewportWidget_HandlePointerPress(
				*this,
				widgetRect,
				touch.position,
				Std::Opt<u8>{ touch.id });
		}
	}

	if (touch.type == Gui::TouchEventType::Moved)
	{
	  if (this->isCurrentlyHoldingGizmo && this->gizmo_touchID.HasValue() && this->gizmo_touchID.Value() == touch.id)
	  {
			if (this->editorImpl->selectedEntity.HasValue())
			{
				Entity entity = editorImpl->selectedEntity.Value();
				// First check if we have a transform
				Transform* transformPtr = editorImpl->scene->GetComponent<Transform>(entity);
				if (transformPtr != nullptr)
				{
					Transform& transform = *transformPtr;
					impl::TranslateAlongGizmoAxis(
						*this,
						widgetRect,
						touch.position,
						transform);
				}
			}
		}

		for (auto& joystick : joysticks)
		{
			if (joystick.touchID.HasValue() && joystick.touchID.Value() == touch.id)
			{
				joystick.currentPosition = { (i32)Math::Round(touch.position.x), (i32)Math::Round(touch.position.y) };
				Math::Vec2Int relativeVector = joystick.currentPosition - joystick.originPosition;
				Math::Vec2 relativeVectorFloat = { (f32)relativeVector.x, (f32)relativeVector.y };
				if (relativeVectorFloat.MagnitudeSqrd() >= Math::Sqrd(joystickPixelRadius))
				{
					relativeVectorFloat = relativeVectorFloat.GetNormalized() * (f32)joystickPixelRadius;
					joystick.currentPosition.x = joystick.originPosition.x + (i32)Math::Round(relativeVectorFloat.x);
					joystick.currentPosition.y = joystick.originPosition.y + (i32)Math::Round(relativeVectorFloat.y);
				}
			}
		}
	}
	else if (touch.type == Gui::TouchEventType::Up)
	{
		if (this->isCurrentlyHoldingGizmo && this->gizmo_touchID.HasValue() && this->gizmo_touchID.Value() == touch.id)
		{
			this->isCurrentlyHoldingGizmo = false;
			this->gizmo_touchID = Std::nullOpt;
		}

		for (auto& joystick : joysticks)
		{
			if (joystick.touchID.HasValue() && joystick.touchID.Value() == touch.id)
			{
				joystick.touchID = Std::nullOpt;
				joystick.currentPosition = joystick.originPosition;
				break;
			}
		}
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
	this->isVisible = true;
	if (!extentsAreInitialized)
	{
		currentExtent = widgetRect.extent;
		newExtent = widgetRect.extent;
		extentsAreInitialized = true;
	}
	else
	{
		if (newExtent != widgetRect.extent)
		{
			currentlyResizing = true;
			newExtent = widgetRect.extent;
		}
		else
		{
			if (currentlyResizing)
			{
				currentlyResizing = false;
				extentCorrectTickCounter = 0;
			}
			newExtent = widgetRect.extent;
			extentCorrectTickCounter += 1;
			if (extentCorrectTickCounter >= 15)
				currentExtent = newExtent;
		}
	}

	// First draw the viewport.
	Gfx::GuiDrawCmd drawCmd{};
	drawCmd.type = Gfx::GuiDrawCmd::Type::Viewport;
	drawCmd.viewport.id = viewportId;
	drawCmd.rectPosition.x = (f32)widgetRect.position.x / framebufferExtent.width;
	drawCmd.rectPosition.y = (f32)widgetRect.position.y / framebufferExtent.height;
	drawCmd.rectExtent.x = (f32)widgetRect.extent.width / framebufferExtent.width;
	drawCmd.rectExtent.y = (f32)widgetRect.extent.height / framebufferExtent.height;
	drawInfo.drawCmds.push_back(drawCmd);

	// Draw a circle, start from the top, move clockwise
	Gfx::GuiDrawCmd::MeshSpan circleMeshSpan{};
	{
		u32 circleVertexCount = 30;
		circleMeshSpan.vertexOffset = (u32)drawInfo.vertices.size();
		circleMeshSpan.indexOffset = (u32)drawInfo.indices.size();
		circleMeshSpan.indexCount = circleVertexCount * 3;
		// Create the vertices, we insert the middle vertex first.
		drawInfo.vertices.push_back({});
		for (u32 i = 0; i < circleVertexCount; i++)
		{
			Gfx::GuiVertex newVertex{};
			f32 currentRadians = 2 * Math::pi / circleVertexCount * i;
			newVertex.position.x += Math::Sin(currentRadians);
			newVertex.position.y += Math::Cos(currentRadians);
			drawInfo.vertices.push_back(newVertex);
		}
		// Build indices
		for (u32 i = 0; i < circleVertexCount - 1; i++)
		{
			drawInfo.indices.push_back(i + 1);
			drawInfo.indices.push_back(0);
			drawInfo.indices.push_back(i + 2);
		}
		drawInfo.indices.push_back(circleVertexCount);
		drawInfo.indices.push_back(0);
		drawInfo.indices.push_back(1);
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
			drawInfo.drawCmds.push_back(cmd);
		}

		Gfx::GuiDrawCmd cmd{};
		cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
		cmd.filledMesh.color = { 1.f, 1.f, 1.f, 0.75f };
		cmd.filledMesh.mesh = circleMeshSpan;
		cmd.rectPosition.x = (f32)joystick.currentPosition.x / framebufferExtent.width;
		cmd.rectPosition.y = (f32)joystick.currentPosition.y / framebufferExtent.height;
		cmd.rectExtent.x = (f32)joystickPixelRadius / framebufferExtent.width;
		cmd.rectExtent.y = (f32)joystickPixelRadius / framebufferExtent.height;
		drawInfo.drawCmds.push_back(cmd);
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