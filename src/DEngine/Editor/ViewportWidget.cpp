#include "ViewportWidget.hpp"

#include <DEngine/Std/Trait.hpp>

#include <iostream> // For testing

using namespace DEngine;
using namespace DEngine::Editor;

Math::Vec3 Gizmo::DeprojectScreenspacePoint(
	Math::Vec2 screenSpacePosNormalized,
	Math::Mat4 projectionMatrix) noexcept
{
	Math::Vec4 vector = screenSpacePosNormalized.AsVec4(1.f, 1.f);
	vector = projectionMatrix.GetInverse().Value() * vector;
	for (auto& item : vector)
		item /= vector.w;
	return Math::Vec3(vector);
}

f32 Gizmo::ComputeScale(
	Math::Mat4 const& worldTransform,
	u32 targetSizePx,
	Math::Mat4 const& projection,
	Gui::Extent viewportSize) noexcept
{
	f32 const pixelSize = 1.f / viewportSize.height;
	Math::Vec4 zVec = { worldTransform.At(3, 0), worldTransform.At(3, 1), worldTransform.At(3, 2), worldTransform.At(3, 3) };
	return targetSizePx * pixelSize * (projection * zVec).w;
}

Std::Opt<f32> Gizmo::IntersectRayPlane(
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

Std::Opt<f32> Gizmo::IntersectRayCylinder(
	Test_Ray ray, 
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
			cylinderAxis.Normalized(),
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

Std::Opt<f32> Gizmo::IntersectArrow(
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

Math::Vec3 Gizmo::TranslateAlongPlane(
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

InternalViewportWidget::InternalViewportWidget(
	EditorImpl& implData, 
	Gfx::Context& gfxCtxIn) 
	: gfxCtx(&gfxCtxIn), implData(&implData)
{
	implData.viewportWidgets.push_back(this);

	auto newViewportRef = gfxCtx->NewViewport();
	viewportId = newViewportRef.ViewportID();
}

InternalViewportWidget::~InternalViewportWidget()
{
	gfxCtx->DeleteViewport(viewportId);
	auto ptrIt = Std::FindIf(
		implData->viewportWidgets.begin(),
		implData->viewportWidgets.end(),
		[this](decltype(implData->viewportWidgets)::value_type const& val) -> bool { return val == this; });
	DENGINE_DETAIL_ASSERT(ptrIt != implData->viewportWidgets.end());
	implData->viewportWidgets.erase(ptrIt);
}

Math::Vec2 InternalViewportWidget::GetNormalizedViewportPosition(
	Math::Vec2Int cursorPos,
	Gui::Rect viewportRect) noexcept
{
	Math::Vec2 cursorNormalizedPos = {
		(f32)(cursorPos.x - viewportRect.position.x) / (f32)viewportRect.extent.width,
		(f32)(cursorPos.y - viewportRect.position.y) / (f32)viewportRect.extent.height };
	cursorNormalizedPos *= 2.f;
	cursorNormalizedPos -= { 1.f, 1.f };
	cursorNormalizedPos.y = -cursorNormalizedPos.y;
	return cursorNormalizedPos;
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

// return value is the axis index
namespace DEngine::Editor
{
	struct GizmoHitTestReturnType
	{
		Gizmo::GizmoPart part{};
		Gizmo::Test_Ray ray{};
	};

	static Std::Opt<GizmoHitTestReturnType> GizmoHitTest(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		Math::Vec2Int cursorPos,
		Transform const& transform);

	static void TranslateAlongGizmoAxis(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		Math::Vec2Int cursorPos,
		Transform& transform);
}
static Std::Opt<GizmoHitTestReturnType> Editor::GizmoHitTest(
	InternalViewportWidget const& widget, 
	Gui::Rect widgetRect, 
	Math::Vec2Int cursorPos,
	Transform const& transform)
{
	Math::Mat4 worldTransform = Math::Mat4::Identity();
	Math::LinTran3D::SetTranslation(worldTransform, { transform.position.x, transform.position.y, 0.f });

	f32 aspectRatio = (f32)widgetRect.extent.width / (f32)widgetRect.extent.height;
	Math::Mat4 projMat = widget.GetProjectionMatrix(aspectRatio);

	u32 smallestViewportExtent = Math::Min(widgetRect.extent.width, widgetRect.extent.height);
	f32 scale = Gizmo::ComputeScale(
		worldTransform,
		smallestViewportExtent * Gizmo::defaultGizmoSizeRelative,
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
	Math::Vec3 vector = Gizmo::DeprojectScreenspacePoint(InternalViewportWidget::GetNormalizedViewportPosition(cursorPos, widgetRect), projMat);
	ray.direction = (vector - widget.cam.position).Normalized();
	
	Std::Opt<GizmoHitTestReturnType> returnVal{};
	
	bool gizmoPartHit = false;
	f32 closestDistance = 0.f;
	Gizmo::GizmoPart gizmoPart{};
	{
		// Next we handle the X arrow
		Math::Mat4 worldTransform = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::Y, Math::pi / 2);
		Math::LinTran3D::SetTranslation(worldTransform, Math::Vec3{ transform.position.x, transform.position.y, 0.f });

		Std::Opt<f32> hit = Gizmo::IntersectArrow(ray, arrow, worldTransform);
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

		Std::Opt<f32> hit = Gizmo::IntersectArrow(ray, arrow, worldTransform);
		if (hit.HasValue() && (!gizmoPartHit || hit.Value() < closestDistance))
		{
			closestDistance = hit.Value();
			gizmoPartHit = true;
			gizmoPart = Gizmo::GizmoPart::ArrowY;
		}
	}

	// Then we check the XY quad
	{
		Std::Opt<f32> hit = Gizmo::IntersectRayPlane(ray, Math::Vec3{ 0.f, 0.f, 1.f }, transform.position);
		if (hit.HasValue())
		{
			f32 planeScale = scale * Gizmo::defaultPlaneScaleRelative;
			f32 planeOffset = scale * Gizmo::defaultPlaneOffsetRelative;

			Math::Vec3 quadPosition = transform.position + Math::Vec3{ 1.f, 1.f, 0.f } * planeOffset;

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

static void DEngine::Editor::TranslateAlongGizmoAxis(
	InternalViewportWidget const& widget,
	Gui::Rect widgetRect,
	Math::Vec2Int cursorPos,
	Transform& transform)
{
	Math::Vec2 cursorNormalizedPos = InternalViewportWidget::GetNormalizedViewportPosition(cursorPos, widgetRect);
	Math::Mat4 projMat = widget.GetProjectionMatrix((f32)widgetRect.extent.width / (f32)widgetRect.extent.height);
	Math::Vec3 rayDir = (Gizmo::DeprojectScreenspacePoint(cursorNormalizedPos, projMat) - widget.cam.position).Normalized();

	Gizmo::Test_Ray ray{};
	ray.direction = rayDir;
	ray.origin = widget.cam.position;

	Math::Vec3 newPos = Gizmo::TranslateAlongPlane(ray, Math::Vec3::Forward(), widget.gizmo_initialPos, widget.gizmo_initialRay);
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

	if (!isCurrentlyHoldingGizmo && event.clicked && event.button == Gui::CursorButton::Primary && cursorIsInside && implData->selectedEntity.HasValue())
	{
		Entity entity = implData->selectedEntity.Value();
		// First check if we have a transform
		Transform const* transformPtr = implData->scene->GetComponent<Transform>(entity);
		if (transformPtr != nullptr)
		{
			Transform const& transform = *transformPtr;

			auto hitResult = GizmoHitTest(
				*this,
				widgetRect,
				cursorPos,
				transform);

			if (hitResult.HasValue())
			{
				auto const& hit = hitResult.Value();
				this->isCurrentlyHoldingGizmo = true;
				this->gizmo_initialRay = hit.ray;
				this->gizmo_initialPos = transform.position;
				this->gizmo_holdingPart = hit.part;
			}
		}
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
		if (this->implData->selectedEntity.HasValue())
		{
			Entity entity = implData->selectedEntity.Value();
			// First check if we have a transform
			Transform* transformPtr = implData->scene->GetComponent<Transform>(entity);
			if (transformPtr != nullptr)
			{
				Transform& transform = *transformPtr;

				TranslateAlongGizmoAxis(
					*this,
					widgetRect,
					event.position,
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
				relativeVectorFloat = relativeVectorFloat.Normalized() * (f32)joystickPixelRadius;
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

	if (touch.type == Gui::TouchEventType::Down)
	{
		bool hitSomething = false;
		for (auto& joystick : joysticks)
		{
			if (hitSomething)
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
				hitSomething = true;
			}
		}

		// We didn't hit any of the joysticks, check if we hit any of the gizmo arrows.
		if (!hitSomething && !isCurrentlyHoldingGizmo && this->implData->selectedEntity.HasValue())
		{
			Entity entity = implData->selectedEntity.Value();
			// First check if we have a transform
			Transform* transformPtr = implData->scene->GetComponent<Transform>(entity);
			if (transformPtr != nullptr)
			{
				Transform const& transform = *transformPtr;

				auto hitResult = GizmoHitTest(
					*this,
					widgetRect,
					{ (i32)touch.position.x, (i32)touch.position.y },
					transform);

				if (hitResult.HasValue())
				{
					auto const& hit = hitResult.Value();
					this->isCurrentlyHoldingGizmo = true;
					this->gizmo_touchID = { touch.id };
					this->gizmo_initialRay = hit.ray;
					this->gizmo_initialPos = transform.position;
					this->gizmo_holdingPart = hit.part;
				}
			}
		}
	}

	if (touch.type == Gui::TouchEventType::Moved)
	{
	    if (this->isCurrentlyHoldingGizmo && this->gizmo_touchID.HasValue() && this->gizmo_touchID.Value() == touch.id)
	    {
				if (this->implData->selectedEntity.HasValue())
				{
					Entity entity = implData->selectedEntity.Value();
					// First check if we have a transform
					Transform* transformPtr = implData->scene->GetComponent<Transform>(entity);
					if (transformPtr != nullptr)
					{
						Transform& transform = *transformPtr;
						TranslateAlongGizmoAxis(
							*this,
							widgetRect,
							{ (i32)touch.position.x, (i32)touch.position.y },
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
					relativeVectorFloat = relativeVectorFloat.Normalized() * (f32)joystickPixelRadius;
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