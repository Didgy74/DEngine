#include <DEngine/Editor/Joystick.hpp>

#include <DEngine/Math/Constant.hpp>
#include <DEngine/Math/Trigonometric.hpp>

#include <DEngine/Gui/DrawInfo.hpp>

namespace DEngine::Editor::impl
{
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

	static constexpr u8 cursorPointerId = (u8)-1;

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
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};

	[[nodiscard]] static bool Joystick_PointerPress(
		Joystick& widget,
		Gui::Rect const& widgetRect,
		Gui::Rect const& visibleRect,
		PointerPress_Pointer const& pointer,
		bool eventConsumed)
	{
		if (eventConsumed && pointer.pressed)
			return false;

		if (pointer.pressed && widget.pressedData.HasValue())
			return false;

		if (pointer.type != PointerType::Primary)
			return false;

		if (widget.pressedData.HasValue())
		{
			auto const activeData = widget.pressedData.Value();
			if (activeData.pointerId == pointer.id && !pointer.pressed)
			{
				// Disactivate the thing
				widget.pressedData = Std::nullOpt;
				return false;
			}
		}

		if (!pointer.pressed)
			return false;

		// Calculate distance from center
		auto outerRadius = Math::Min(widgetRect.extent.width, widgetRect.extent.height) / 2;
		auto widgetCenter = widgetRect.position + Math::Vec2Int{ (i32)outerRadius, (i32)outerRadius };
		auto relativeVec = pointer.pos - Math::Vec2{ (f32)widgetCenter.x, (f32)widgetCenter.y };
		bool pointerIsInside = relativeVec.MagnitudeSqrd() <= (f32)Math::Sqrd(outerRadius);
		if (!pointerIsInside && pointer.pressed)
			return false;

		Joystick::PressedData newData = {};
		newData.pointerId = pointer.id;
		newData.currPos = GetNormalizedVec(widgetRect, pointer.pos);

		widget.pressedData = newData;

		return true;
	}

	[[nodiscard]] static bool Joystick_PointerMove(
		Joystick& widget,
		Gui::Rect const& widgetRect,
		u8 pointerId,
		Math::Vec2 pointerPos)
	{
		if (!widget.pressedData.HasValue())
			return false;
		auto& activeData = widget.pressedData.Value();
		if (activeData.pointerId != pointerId)
			return false;

		activeData.currPos = GetNormalizedVec(widgetRect, pointerPos);

		return true;
	}
}

using namespace DEngine;
using namespace DEngine::Editor;

Gui::SizeHint Joystick::GetSizeHint2(Gui::Widget::GetSizeHint2_Params const& params) const
{
	Gui::SizeHint returnVal = {};
	returnVal.expandX = false;
	returnVal.expandY = false;
	returnVal.minimum = { 150, 150 };

	auto& pusher = params.pusher;

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnVal);
	return returnVal;
}

void Joystick::Render2(
	Gui::Widget::Render_Params const& params,
	Gui::Rect const& widgetRect,
	Gui::Rect const& visibleRect) const
{
	// Draw a circle, start from the top, move clockwise
	Gfx::GuiDrawCmd::MeshSpan circleMeshSpan = {};

	u32 circleVertexCount = 30;
	circleMeshSpan.vertexOffset = (u32)params.drawInfo.vertices->size();
	circleMeshSpan.indexOffset = (u32)params.drawInfo.indices->size();
	circleMeshSpan.indexCount = circleVertexCount * 3;
	// Create the vertices, we insert the middle vertex first.
	params.drawInfo.vertices->push_back({ 0.5f, 0.5f });
	for (u32 i = 0; i < circleVertexCount; i++)
	{
		f32 currentRadians = 2 * Math::pi / circleVertexCount * i;
		Gfx::GuiVertex newVertex = {};
		newVertex.position.x = Math::Sin(currentRadians);
		newVertex.position.y = Math::Cos(currentRadians);
		// Translate from [-1, 1] space to [0, 1]
		newVertex.position += { 1.f, 1.f };
		newVertex.position *= 0.5f;
		params.drawInfo.vertices->push_back(newVertex);
	}
	// Build indices
	for (u32 i = 0; i < circleVertexCount - 1; i++)
	{
		params.drawInfo.indices->push_back(i + 1);
		params.drawInfo.indices->push_back(0);
		params.drawInfo.indices->push_back(i + 2);
	}
	params.drawInfo.indices->push_back(circleVertexCount);
	params.drawInfo.indices->push_back(0);
	params.drawInfo.indices->push_back(1);

	auto outerDiameter = Math::Min(widgetRect.extent.width, widgetRect.extent.height);

	Gfx::GuiDrawCmd cmd = {};
	cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
	cmd.filledMesh.color = { 0.f, 0.f, 0.f, 0.25f };
	cmd.filledMesh.mesh = circleMeshSpan;
	cmd.rectPosition.x = (f32)widgetRect.position.x / params.framebufferExtent.width;
	cmd.rectPosition.y = (f32)widgetRect.position.y / params.framebufferExtent.height;
	cmd.rectExtent.x = (f32)outerDiameter / params.framebufferExtent.width;
	cmd.rectExtent.y = (f32)outerDiameter / params.framebufferExtent.height;
	params.drawInfo.drawCmds->push_back(cmd);

	auto innerDiameter = outerDiameter / 2;

	//Gfx::GuiDrawCmd cmd{};
	Math::Vec2Int innerCirclePos = {};
	Math::Vec2 currPos = GetVector();
	// Translate from space [-1, 1] to [-outerDiameter, outerDiameter]
	currPos *= outerDiameter / 2.f;
	// Clamp length to inner-diameter
	if (currPos.MagnitudeSqrd() >= Math::Sqrd(innerDiameter / 2.f))
		currPos = currPos.GetNormalized() * (innerDiameter / 2.f);
	innerCirclePos += { (i32)currPos.x, (i32)currPos.y };
	innerCirclePos += widgetRect.position;
	innerCirclePos += { (i32)outerDiameter / 2, (i32)outerDiameter / 2 };


	innerCirclePos -= { (i32)innerDiameter / 2, (i32)innerDiameter / 2 };

	cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
	cmd.filledMesh.color = { 1.f, 1.f, 1.f, 0.75f };
	cmd.filledMesh.mesh = circleMeshSpan;
	cmd.rectPosition.x = (f32)innerCirclePos.x / params.framebufferExtent.width;
	cmd.rectPosition.y = (f32)innerCirclePos.y / params.framebufferExtent.height;
	cmd.rectExtent.x = (f32)outerDiameter * 0.5f / params.framebufferExtent.width;
	cmd.rectExtent.y = (f32)outerDiameter * 0.5f / params.framebufferExtent.height;
	params.drawInfo.drawCmds->push_back(cmd);
}

bool Joystick::CursorPress2(
	Gui::Widget::CursorPressParams const& params,
	Gui::Rect const& widgetRect,
	Gui::Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.type = impl::ToPointerType(params.event.button);
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.pressed = params.event.pressed;

	return impl::Joystick_PointerPress(
		*this,
		widgetRect,
		visibleRect,
		pointer,
		consumed);
}

bool Joystick::CursorMove(
	Gui::Widget::CursorMoveParams const& params,
	Gui::Rect const& widgetRect,
	Gui::Rect const& visibleRect,
	bool occluded)
{
	return impl::Joystick_PointerMove(
		*this,
		widgetRect,
		impl::cursorPointerId,
		{ (f32)params.event.position.x, (f32)params.event.position.y });
}
