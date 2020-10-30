#pragma once

#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/Button.hpp>

#include "ContextImpl.hpp"

#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/Std/Utility.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>

namespace DEngine::Editor
{
	class InternalViewportWidget : public Gui::Widget
	{
	public:
		Gfx::ViewportID viewportId = Gfx::ViewportID::Invalid;
		Gfx::Context* gfxCtx = nullptr;
		ContextImpl* implData = nullptr;

		mutable bool isVisible = false;

		mutable Gui::Extent currentExtent{};
		mutable Gui::Extent newExtent{};
		mutable bool currentlyResizing = false;
		mutable u32 extentCorrectTickCounter = 0;
		mutable bool extentsAreInitialized = false;

		bool isCurrentlyClicked = false;

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
			Math::Vec3 position{ 0.f, 0.f, 2.f };
			Math::UnitQuat rotation = Math::UnitQuat::FromEulerAngles(0, 180.f, 0.f);
			f32 verticalFov = 60.f;
		};
		Camera cam{};

		InternalViewportWidget(ContextImpl& implData, Gfx::Context& gfxCtxIn) :
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
				[this](decltype(implData->viewportWidgets.front()) const& val) -> bool { return val == this; });
			DENGINE_DETAIL_ASSERT(ptrIt != implData->viewportWidgets.end());
			implData->viewportWidgets.erase(ptrIt);
		}

		virtual void CursorClick(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Math::Vec2Int cursorPos,
			Gui::CursorClickEvent event) override
		{
			UpdateJoystickOrigin(widgetRect);

			if (event.button == Gui::CursorButton::Right && event.clicked && !isCurrentlyClicked)
			{
				bool isInside = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
				if (isInside)
				{
					isCurrentlyClicked = true;
					App::LockCursor(true);
				}
			}

			if (event.button == Gui::CursorButton::Right && !event.clicked && isCurrentlyClicked)
			{
				isCurrentlyClicked = false;
				App::LockCursor(false);
			}

			UpdateJoystickOrigin(widgetRect);
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

		virtual void CursorMove(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::CursorMoveEvent event) override
		{
			UpdateJoystickOrigin(widgetRect);

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

		virtual void TouchEvent(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::TouchEvent touch) override
		{
			UpdateJoystickOrigin(widgetRect);
			if (touch.type == Gui::TouchEventType::Down)
			{
				for (auto& joystick : joysticks)
				{
					if (joystick.touchID.HasValue())
						continue;
					Math::Vec2Int fingerPos = { (i32)touch.position.x, (i32)touch.position.y };
					Math::Vec2Int relativeVector = fingerPos - joystick.originPosition;
					if (relativeVector.MagnitudeSqrd() <= Math::Sqrd(joystickPixelRadius))
					{
						// Cursor is inside circle
						joystick.touchID = touch.id;
						joystick.currentPosition = fingerPos;
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

		void ApplyCameraRotation(Math::Vec2 input)
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

		void ApplyCameraMovement(Math::Vec3 move, f32 speed)
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

		Math::Vec2 GetLeftJoystickVec() const
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

		Math::Vec2 GetRightJoystickVec() const
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

		void TickTest(f32 deltaTime)
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
						ApplyCameraRotation(Math::Vec2{ relativeVectorFloat.x, -relativeVectorFloat.y } * sensitivity * deltaTime);
					}
				}
				else
					joystick.currentPosition = joystick.originPosition;
			}
		}

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
			Gui::DrawInfo& drawInfo) const override
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

		// Pixel space
		void UpdateJoystickOrigin(Gui::Rect widgetRect) const
		{
			joysticks[0].originPosition.x = widgetRect.position.x + joystickPixelRadius * 2;
			joysticks[0].originPosition.y = widgetRect.position.y + widgetRect.extent.height - joystickPixelRadius * 2;

			joysticks[1].originPosition.x = widgetRect.position.x + widgetRect.extent.width - joystickPixelRadius * 2;
			joysticks[1].originPosition.y = widgetRect.position.y + widgetRect.extent.height - joystickPixelRadius * 2;
		}
	};


	class ViewportWidget : public Gui::StackLayout 
	{
	public:
		ViewportWidget(ContextImpl& implData, Gfx::Context& ctx) :
			Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
		{
			// Generate top navigation bar
			Gui::StackLayout* menuBar = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
			AddLayout2(Std::Box<Gui::Layout>{ menuBar });
			menuBar->color = { 0.25f, 0.25f, 0.25f, 1.f };
			
			Gui::Button* camMenuButton = new Gui::Button;
			menuBar->AddWidget2(Std::Box<Gui::Widget>{ camMenuButton });
			camMenuButton->textWidget.String_Set("Camera");
			camMenuButton->activatePfn = [](
				Gui::Button& btn,
				Gui::Context& ctx,
				Gui::WindowID windowId,
				Gui::Rect widgetRect,
				Gui::Rect visibleRect)
			{
				Gui::StackLayout* cameraList = new Gui::StackLayout(Gui::StackLayout::Direction::Vertical);
				Std::Box<Gui::Layout> layout{ cameraList };

				Gui::Button* thisCamButton = new Gui::Button;
				cameraList->AddWidget2(Std::Box<Gui::Widget>{ thisCamButton });
				thisCamButton->textWidget.String_Set("FreeLook");
				thisCamButton->SetToggled(true);

				ctx.Test_AddMenu(
					windowId,
					Std::Move(layout),
					Gui::Rect{ { widgetRect.position.x, widgetRect.position.y + (i32)widgetRect.extent.height}, {} });
			};


			InternalViewportWidget* viewport = new InternalViewportWidget(implData, ctx);
			AddWidget2(Std::Box<Gui::Widget>{ viewport });
		}
	};
}