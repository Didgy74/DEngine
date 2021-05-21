#include <DEngine/Editor/Joystick.hpp>

#include <DEngine/Math/Constant.hpp>
#include <DEngine/Math/Trigonometric.hpp>

namespace DEngine::Editor::impl
{
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

	[[nodiscard]] static Math::Vec2 GetNormalizedVec(
		Gui::Rect widgetRect,
		Math::Vec2 pointerPos) noexcept
	{
		// Calculate distance from center
		auto outerRadius = Math::Min(widgetRect.extent.width, widgetRect.extent.height) / 2;

		auto widgetCenter = widgetRect.position + Math::Vec2Int{ (i32)outerRadius, (i32)outerRadius };
		auto relativeVec = pointerPos - Math::Vec2{ (f32)widgetCenter.x, (f32)widgetCenter.y };
		relativeVec *= 1.f / outerRadius;
		if (relativeVec.MagnitudeSqrd() >= 1.f)
			relativeVec.Normalize();

		return relativeVec;
	}
	[[nodiscard]] static bool Joystick_PointerPress(
		Joystick& widget,
		Gui::Rect widgetRect,
		Gui::Rect visibleRect,
		u8 pointerId,
		PointerType pointerType,
		Math::Vec2 pointerPos,
		bool pressed)
	{
		if (pointerType != PointerType::Primary)
			return false;

		if (pressed && widget.activeData.HasValue())
			return false;

		if (!pressed && widget.activeData.HasValue() && widget.activeData.Value().pointerId == pointerId)
		{
			// Disactivate the thing
			widget.activeData = Std::nullOpt;
			return false;
		}

		bool pointerIsInside = widgetRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
		if (!pointerIsInside)
			return false;

		// Calculate distance from center
		auto outerRadius = Math::Min(widgetRect.extent.width, widgetRect.extent.height) / 2;
		auto widgetCenter = widgetRect.position + Math::Vec2Int{ (i32)outerRadius, (i32)outerRadius };

		auto relativeVec = pointerPos - Math::Vec2{ (f32)widgetCenter.x, (f32)widgetCenter.y };
		if (relativeVec.MagnitudeSqrd() > Math::Sqrd(outerRadius))
			return false;

		Joystick::ActiveData newData = {};
		newData.pointerId = pointerId;
		newData.currPos = GetNormalizedVec(widgetRect, pointerPos);

		widget.activeData = newData;

		return true;
	}

	[[nodiscard]] static bool Joystick_PointerMove(
		Joystick& widget,
		Gui::Rect widgetRect,
		u8 pointerId,
		Math::Vec2 pointerPos)
	{
		if (!widget.activeData.HasValue())
			return false;
		auto& activeData = widget.activeData.Value();
		if (activeData.pointerId != pointerId)
			return false;

		activeData.currPos = GetNormalizedVec(widgetRect, pointerPos);

		return true;
	}
}

using namespace DEngine;
using namespace DEngine::Editor;

bool Joystick::CursorPress(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Math::Vec2Int cursorPos,
	Gui::CursorClickEvent event)
{
	return impl::Joystick_PointerPress(
		*this,
		widgetRect,
		visibleRect,
		cursorPointerId,
		impl::ToPointerType(event.button),
		{ (f32)cursorPos.x, (f32)cursorPos.y },
		event.clicked);
}

bool Joystick::CursorMove(
	Gui::Context& ctx,
	Gui::WindowID windowId,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Gui::CursorMoveEvent event,
	bool occluded)
{
	return impl::Joystick_PointerMove(
		*this,
		widgetRect,
		cursorPointerId,
		{ (f32)event.position.x, (f32)event.position.y });
}

Gui::SizeHint Joystick::GetSizeHint(Gui::Context const& ctx) const
{
	Gui::SizeHint returnVal = {};
	returnVal.expandX = false;
	returnVal.expandY = false;
	returnVal.preferred = { 150, 150 };
	return returnVal;
}

void Joystick::Render(
	Gui::Context const& ctx,
	Gui::Extent framebufferExtent,
	Gui::Rect widgetRect,
	Gui::Rect visibleRect,
	Gui::DrawInfo& drawInfo) const
{
	// Draw a circle, start from the top, move clockwise
	Gfx::GuiDrawCmd::MeshSpan circleMeshSpan = {};

	u32 circleVertexCount = 30;
	circleMeshSpan.vertexOffset = (u32)drawInfo.vertices->size();
	circleMeshSpan.indexOffset = (u32)drawInfo.indices->size();
	circleMeshSpan.indexCount = circleVertexCount * 3;
	// Create the vertices, we insert the middle vertex first.
	drawInfo.vertices->push_back({ 0.5f, 0.5f });
	for (u32 i = 0; i < circleVertexCount; i++)
	{
		f32 currentRadians = 2 * Math::pi / circleVertexCount * i;
		Gfx::GuiVertex newVertex = {};
		newVertex.position.x = Math::Sin(currentRadians);
		newVertex.position.y = Math::Cos(currentRadians);
		// Translate from [-1, 1] space to [0, 1]
		newVertex.position += { 1.f, 1.f };
		newVertex.position *= 0.5f;
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

	auto outerDiameter = Math::Min(widgetRect.extent.width, widgetRect.extent.height);

	Gfx::GuiDrawCmd cmd = {};
	cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
	cmd.filledMesh.color = { 0.f, 0.f, 0.f, 0.25f };
	cmd.filledMesh.mesh = circleMeshSpan;
	cmd.rectPosition.x = (f32)widgetRect.position.x / framebufferExtent.width;
	cmd.rectPosition.y = (f32)widgetRect.position.y / framebufferExtent.height;
	cmd.rectExtent.x = (f32)outerDiameter / framebufferExtent.width;
	cmd.rectExtent.y = (f32)outerDiameter / framebufferExtent.height;
	drawInfo.drawCmds->push_back(cmd);

	auto innerDiameter = outerDiameter / 2;

	//Gfx::GuiDrawCmd cmd{};
	Math::Vec2Int innerCirclePos = {};
	if (activeData.HasValue())
	{
		Math::Vec2 currPos = activeData.Value().currPos;
		// Translate from space [-1, 1] to [-outerDiameter, outerDiameter]
		currPos *= outerDiameter / 2.f;
		// Clamp length to inner-diameter
		if (currPos.MagnitudeSqrd() >= Math::Sqrd(innerDiameter / 2.f))
			currPos = currPos.GetNormalized() * (innerDiameter / 2.f);
		innerCirclePos += { (i32)currPos.x, (i32)currPos.y };
	}
	innerCirclePos += widgetRect.position;
	innerCirclePos += { (i32)outerDiameter / 2, (i32)outerDiameter / 2 };


	innerCirclePos -= { (i32)innerDiameter / 2, (i32)innerDiameter / 2 };

	cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
	cmd.filledMesh.color = { 1.f, 1.f, 1.f, 0.75f };
	cmd.filledMesh.mesh = circleMeshSpan;
	cmd.rectPosition.x = (f32)innerCirclePos.x / framebufferExtent.width;
	cmd.rectPosition.y = (f32)innerCirclePos.y / framebufferExtent.height;
	cmd.rectExtent.x = (f32)outerDiameter * 0.5f / framebufferExtent.width;
	cmd.rectExtent.y = (f32)outerDiameter * 0.5f / framebufferExtent.height;
	drawInfo.drawCmds->push_back(cmd);
}