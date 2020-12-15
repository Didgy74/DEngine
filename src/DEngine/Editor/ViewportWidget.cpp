#include "ViewportWidget.hpp"

#include <iostream> // For testing

using namespace DEngine;
using namespace DEngine::Editor;

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
		u8 axis;
		anton::math::Ray ray;
	};

	static Std::Opt<GizmoHitTestReturnType> GizmoHitTest(
		InternalViewportWidget const& widget,
		Gui::Rect widgetRect,
		Math::Vec2Int cursorPos,
		Transform const& transform);
}
static Std::Opt<GizmoHitTestReturnType> Editor::GizmoHitTest(
	InternalViewportWidget const& widget, 
	Gui::Rect widgetRect, 
	Math::Vec2Int cursorPos,
	Transform const& transform)
{
	Math::Mat4 worldTransform = Math::Mat4::Identity();
	Math::LinTran3D::SetTranslation(worldTransform, { transform.position.x, transform.position.y, 0.f });
	anton::math::Mat4 anton_worldTransform{ worldTransform.Data() };

	f32 aspectRatio = (f32)widgetRect.extent.width / (f32)widgetRect.extent.height;
	Math::Mat4 projMat = widget.GetProjectionMatrix(aspectRatio);
	anton::math::Mat4 anton_projMatrix{ projMat.Data() };

	u32 smallestViewportExtent = Math::Min(widgetRect.extent.width, widgetRect.extent.height);
	f32 scale = anton::gizmo::compute_scale(
		anton_worldTransform,
		smallestViewportExtent / 4,
		anton_projMatrix,
		{ (f32)widgetRect.extent.width, (f32)widgetRect.extent.height });

	// Anton gizmo cannot include scale in the world transform, so we modify the arrow struct
	// to account for the scaling
	anton::gizmo::Arrow_3D arrow{};
	arrow.cap_length = 0.33f;
	arrow.cap_size = 0.2f;
	arrow.draw_style = anton::gizmo::Arrow_3D_Style::cone;
	arrow.shaft_diameter = 0.1f;
	arrow.shaft_length = 0.66f;
	arrow.cap_length *= scale;
	arrow.cap_size *= scale;
	arrow.shaft_diameter *= scale;
	arrow.shaft_length *= scale;

	anton::math::Ray ray{};
	ray.origin = { widget.cam.position.x, widget.cam.position.y, widget.cam.position.z };
	Math::Vec2 cursorNormalizedPos = InternalViewportWidget::GetNormalizedViewportPosition(cursorPos, widgetRect);
	Math::Vec4 vector = Math::Vec4{
		cursorNormalizedPos.x,
		cursorNormalizedPos.y,
		1.f,
		1.f };
	vector = projMat.GetInverse().Value() * vector;
	for (auto& item : vector)
		item /= vector.w;
	Math::Vec3 vec2 = (vector.AsVec3() - widget.cam.position).Normalized();
	ray.direction = { vec2.x, vec2.y, vec2.z };
	
	bool axisWasHit = false;
	f32 closestDistance = 0.f;
	u8 axisIndex = 0;
	{
		// Next we handle the X arrow
		Math::Mat4 worldTransform = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::Y, -Math::pi / 2);
		Math::LinTran3D::SetTranslation(worldTransform, { transform.position.x, transform.position.y, 0.f });
		anton::math::Mat4 anton_worldTransform = anton::math::Mat4{ worldTransform.Data() };
		anton::Optional<anton::f32> hit = anton::gizmo::intersect_arrow_3d(ray, arrow, anton_worldTransform);
		if (hit.holds_value() && (!axisWasHit || hit.value() < closestDistance))
		{
			closestDistance = hit.value();
			axisWasHit = true;
			axisIndex = 0;
		}
	}
	{
		// Next we handle the Y arrow
		Math::Mat4 worldTransform = Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::X, Math::pi / 2);
		Math::LinTran3D::SetTranslation(worldTransform, { transform.position.x, transform.position.y, 0.f });
		anton::math::Mat4 anton_worldTransform = anton::math::Mat4{ worldTransform.Data() };
		anton::Optional<anton::f32> hit = anton::gizmo::intersect_arrow_3d(ray, arrow, anton_worldTransform);
		if (hit.holds_value() && (!axisWasHit || hit.value() < closestDistance))
		{
			closestDistance = hit.value();
			axisWasHit = true;
			axisIndex = 1;
		}
	}

	if (axisWasHit)
	{
		GizmoHitTestReturnType returnVal;
		returnVal.axis = axisIndex;
		returnVal.ray = ray;
		return Std::Opt{ returnVal };
	}
	else
		return Std::nullOpt;
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

	if (isCurrentlyHoldingGizmo && event.button == Gui::CursorButton::Left && !event.clicked)
	{
		isCurrentlyHoldingGizmo = false;
	}

	if (!isCurrentlyHoldingGizmo && event.clicked && event.button == Gui::CursorButton::Left && cursorIsInside && implData->selectedEntity.HasValue())
	{
		Entity entity = implData->selectedEntity.Value();
		// First check if we have a transform
		auto const transformIt = Std::FindIf(
			implData->scene->transforms.begin(),
			implData->scene->transforms.end(),
			[entity](decltype(implData->scene->transforms[0]) const& val) -> bool { return val.a == entity; });
		if (transformIt != implData->scene->transforms.end())
		{
			Transform const& transform = transformIt->b;

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
				this->gizmo_holdingAxis = hit.axis;
			}
		}
	}

	if (event.button == Gui::CursorButton::Right && !event.clicked && isCurrentlyClicked)
	{
		isCurrentlyClicked = false;
		App::LockCursor(false);
	}

	if (event.button == Gui::CursorButton::Right && event.clicked && !isCurrentlyClicked && cursorIsInside)
	{
		isCurrentlyClicked = true;
		App::LockCursor(true);
	}

	if (event.button == Gui::CursorButton::Left && event.clicked)
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

	if (event.button == Gui::CursorButton::Left && !event.clicked)
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
		Math::Vec2 cursorNormalizedPos = GetNormalizedViewportPosition(event.position, widgetRect);
		Math::Vec4 vector = Math::Vec4{
			cursorNormalizedPos.x,
			cursorNormalizedPos.y,
			1.f,
			1.f };
		f32 aspectRatio = (f32)widgetRect.extent.width / (f32)widgetRect.extent.height;
		Math::Mat4 projMat = GetProjectionMatrix(aspectRatio);
		vector = projMat.GetInverse().Value() * vector;
		for (auto& item : vector)
			item /= vector.w;
		Math::Vec3 vec2 = (vector.AsVec3() - cam.position).Normalized();

		anton::math::Ray ray{};
		ray.direction = { vec2.x, vec2.y, vec2.z };
		ray.origin = { cam.position.x, cam.position.y, cam.position.z };
		anton::math::Vec3 anton_initialPos = { this->gizmo_initialPos.x, this->gizmo_initialPos.y, this->gizmo_initialPos.z };
		anton::math::Vec3 anton_axis;
		switch (this->gizmo_holdingAxis)
		{
		case 0:
			anton_axis = { 1.f, 0.f, 0.f };
			break;
		case 1:
			anton_axis = { 0.f, 1.f, 0.f };
			break;
		default:
			DENGINE_DETAIL_UNREACHABLE();
		}
		auto anton_newPos = anton::gizmo::translate_along_line(ray, anton_axis, anton_initialPos, this->gizmo_initialRay);
		// Find the transform component
		Entity selectedEntity = this->implData->selectedEntity.Value();
		auto const transformIt = Std::FindIf(
			this->implData->scene->transforms.begin(),
			this->implData->scene->transforms.end(),
			[selectedEntity](decltype(this->implData->scene->transforms[0]) const& val) -> bool { return val.a == selectedEntity; });
		Transform& transform = transformIt->b;
		transform.position = { anton_newPos.x, anton_newPos.y, anton_newPos.z };
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
		if (!hitSomething && isCurrentlyHoldingGizmo && this->implData->selectedEntity.HasValue())
		{
			Entity entity = implData->selectedEntity.Value();
			// First check if we have a transform
			auto const transformIt = Std::FindIf(
				implData->scene->transforms.begin(),
				implData->scene->transforms.end(),
				[entity](decltype(implData->scene->transforms[0]) const& val) -> bool { return val.a == entity; });
			if (transformIt != implData->scene->transforms.end())
			{
				Transform const& transform = transformIt->b;

				auto hitResult = GizmoHitTest(
					*this,
					widgetRect,
					{ (i32)touch.position.x, (i32)touch.position.y },
					transform);

				if (hitResult.HasValue())
				{
					auto const& hit = hitResult.Value();
					this->isCurrentlyHoldingGizmo = true;
					this->gizmo_initialRay = hit.ray;
					this->gizmo_initialPos = transform.position;
					this->gizmo_holdingAxis = hit.axis;
				}
			}
		}
	}

	if (touch.type == Gui::TouchEventType::Moved)
	{
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