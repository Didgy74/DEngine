#include "Editor.hpp"
#include "DEngine/InputRaw.hpp"
#include "DEngine/Time.hpp"
#include "DEngine/Math/LinearTransform3D.hpp"

#include "DEngine/Application.hpp"

#include "ImGui/imgui.h"

#include <iostream>

namespace DEngine::Editor
{
	static void DrawViewportManager(EditorData& editorData)
	{
		if (ImGui::Begin("Testerrr", nullptr))
		{
			if (ImGui::Button("New viewport!"))
			{
				uSize availableID = static_cast<uSize>(-1);
				uSize insertionIndex = static_cast<uSize>(-1);
				if (editorData.viewports.size() > 0 && editorData.viewports.front().a > 0)
				{
					availableID = 0;
					insertionIndex = 0;
				}
				else
				{
					for (size_t i = 1; i < editorData.viewports.size(); i++)
					{
						auto const& left = editorData.viewports[i - 1];
						auto const& right = editorData.viewports[i];

						if (right.a > left.a + 1)
						{
							availableID = left.a + 1;
							insertionIndex = i;
							break;
						}
					}
				}

				if (insertionIndex == Viewport::invalidCamID)
				{
					Viewport newViewport{};
					newViewport.cameraID = Viewport::invalidCamID;
					uSize id = editorData.viewports.size();
					editorData.viewports.push_back({ id, newViewport });
				}
				else
				{
					Viewport newViewport{};
					newViewport.cameraID = Viewport::invalidCamID;
					uSize id = availableID;
					editorData.viewports.insert(editorData.viewports.begin() + insertionIndex, { id, newViewport });
				}
			}
		}
		ImGui::End();
	}

	static void DrawCameraManager(EditorData& editorData)
	{
		ImGui::SetNextWindowSize({ 250, 250 }, ImGuiCond_FirstUseEver);
		ImGuiWindowFlags windowFlags = 0;
		windowFlags |= ImGuiWindowFlags_::ImGuiWindowFlags_NoResize;
		if (ImGui::Begin("List of cameras"))
		{
			// Draw Camera info widgets
			for (uSize i = 0; i < editorData.cameras.size(); i += 1)
			{
				ImGui::PushID((int)i);

				u64 camID = editorData.cameras[i].a;
				Camera* cam = &editorData.cameras[i].b;

				std::string cameraName = std::string("Camera #") + std::to_string(camID);
				ImGui::Text("%s", cameraName.data());


				if (ImGui::Button("Delete"))
				{
					editorData.cameras.erase(editorData.cameras.begin() + i);
				}

				ImGui::PopID();
			}

			ImVec2 size = ImGui::GetContentRegionAvail();
			if (ImGui::Button("+", { size.x, 0 }))
			{
				// Add a new camera to our array. Figure out which is the next
				// ID we can use and which index to insert into.
				// Or just append if we found no available index
				uSize availableID = static_cast<uSize>(-1);
				uSize insertionIndex = static_cast<uSize>(-1);
				
				if (editorData.cameras.size() > 0 && editorData.cameras.front().a > 0)
				{
					availableID = 0;
					insertionIndex = 0;
				}
				else
				{
					for (size_t i = 1; i < editorData.cameras.size(); i++)
					{
						auto const& left = editorData.cameras[i - 1];
						auto const& right = editorData.cameras[i];

						if (right.a > left.a + 1)
						{
							availableID = left.a + 1;
							insertionIndex = i;
							break;
						}
					}
				}

				if (insertionIndex == static_cast<uSize>(-1))
				{
					Camera newCam{};
					uSize id = editorData.cameras.size();
					editorData.cameras.push_back({ id, newCam });
				}
				else
				{
					Camera newCam{};
					uSize id = availableID;
					editorData.cameras.insert(editorData.cameras.begin() + insertionIndex, { id, newCam });
				}
			}
		}
		ImGui::End();
	}

	static void RenderVirtualViewport_MenuBar(
		Viewport& viewport,
		Cont::Span<Cont::Pair<uSize, Camera> const> cameras)
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Camera"))
			{
				if (ImGui::MenuItem("Free", nullptr, viewport.cameraID == Viewport::invalidCamID))
				{
					viewport.cameraID = Viewport::invalidCamID;
				}

				if (cameras.Size() > 0)
				{
					ImGui::Separator();
					ImGui::MenuItem("Scene #0", nullptr, false, false);

					for (auto const& [camID, camera] : cameras)
					{
						std::string cameraString = std::string("    Camera #") + std::to_string(camID);
						if (ImGui::MenuItem(cameraString.data(), nullptr, viewport.cameraID == camID))
						{
							viewport.cameraID = camID;
						}
					}
					
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Settings"))
			{
				if (ImGui::MenuItem("Paused", nullptr, &viewport.paused))
				{
					//viewport.paused = !viewport.paused;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
	}

	// This also handles camera controls
	static void DrawViewportWidget(
		Viewport& virtualViewport,
		ImVec2 widgetSize,
		Cont::Span<Cont::Pair<uSize, Camera>> cameras)
	{
		ImGui::Image(virtualViewport.gfxViewportRef.ImGuiTexID(), widgetSize);

		if (!virtualViewport.paused && ImGui::IsItemHovered() && Input::Raw::GetValue(Input::Raw::Button::RightMouse))
		{
			if (virtualViewport.currentlyControlling == false)
			{
				//Application::SetRelativeMouseMode(true);
				virtualViewport.currentlyControlling = true;
			}
				
			Camera* cam = nullptr;
			if (virtualViewport.cameraID == Viewport::invalidCamID)
				cam = &virtualViewport.camera;
			else
			{
				for (auto& [camID, camera] : cameras)
				{
					if (camID == virtualViewport.cameraID)
					{
						cam = &camera;
						break;
					}
				}
			}
			assert(cam != nullptr);

			auto delta = Input::Raw::GetMouseDelta();
			
			f32 sensitivity = 0.1f;
			i32 amountX = delta[0];
#pragma warning( suppress : 6011 )
			// Apply left and right rotation
			cam->rotation = Math::UnitQuat::FromVector(Math::Vec3D::Up(), -sensitivity * amountX) * cam->rotation;

			i32 amountY = delta[1];

			// Limit rotation up and down
			Math::Vec3D forward = Math::LinTran3D::ForwardVector(cam->rotation);
			float dot = Math::Vec3D::Dot(forward, Math::Vec3D::Up());
			if (dot <= -0.99f)
				amountY = Math::Max(0, amountY);
			else if (dot >= 0.99f)
				amountY = Math::Min(0, amountY);

			// Apply up and down rotation
			Math::Vec3D right = Math::LinTran3D::RightVector(cam->rotation);
			cam->rotation = Math::UnitQuat::FromVector(right, sensitivity * amountY) * cam->rotation;

			// Handle camera movement
			f32 moveSpeed = 5.f;
			if (Input::Raw::GetValue(Input::Raw::Button::W))
				cam->position -= moveSpeed * Math::LinTran3D::ForwardVector(cam->rotation) * Time::Delta();
			if (Input::Raw::GetValue(Input::Raw::Button::S))
				cam->position += moveSpeed * Math::LinTran3D::ForwardVector(cam->rotation) * Time::Delta();
			if (Input::Raw::GetValue(Input::Raw::Button::D))
				cam->position += moveSpeed * Math::LinTran3D::RightVector(cam->rotation) * Time::Delta();
			if (Input::Raw::GetValue(Input::Raw::Button::A))
				cam->position -= moveSpeed * Math::LinTran3D::RightVector(cam->rotation) * Time::Delta();
			if (Input::Raw::GetValue(Input::Raw::Button::Space))
				cam->position.y -= moveSpeed * Time::Delta();
			if (Input::Raw::GetValue(Input::Raw::Button::LeftCtrl))
				cam->position.y += moveSpeed * Time::Delta();
		}
		else
		{
			if (virtualViewport.currentlyControlling == true)
			{
				//Application::SetRelativeMouseMode(false);
				virtualViewport.currentlyControlling = false;
			}
		}
	}

	static void DrawVirtualViewports(EditorData& editorData, Gfx::Data& gfx)
	{
		// We use signed integer because of the way we remove the viewport.
		for (iSize i = 0; i < (iSize)editorData.viewports.size(); i += 1)
		{
			auto const& viewportID = editorData.viewports[i].a;
			auto& virtualViewport = editorData.viewports[i].b;

			virtualViewport.visible = false;

			// Check that our selected camera still exists
			if (virtualViewport.cameraID != Viewport::invalidCamID)
			{
				bool camFound = false;
				for (auto const& [camID, cam] : editorData.cameras)
				{
					if (camID == virtualViewport.cameraID)
					{
						camFound = true;
						break;
					}
				}
				if (!camFound)
					virtualViewport.cameraID = Viewport::invalidCamID;
			}


			std::string name = std::string("Viewport #") + std::to_string(viewportID);

			bool windowIsOpen = true;

			ImGui::SetNextWindowSize({ 250, 250 }, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints({ 100, 100 }, { 8192, 8192 });
			ImGuiWindowFlags windowFlags = 0;
			windowFlags |= ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse;
			windowFlags |= ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar;
			windowFlags |= ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar;
			if (ImGui::Begin(name.data(), &windowIsOpen, windowFlags))
			{
				virtualViewport.visible = true;
				RenderVirtualViewport_MenuBar(virtualViewport, { editorData.cameras.data(), editorData.cameras.size() });

				ImVec2 newSize = ImGui::GetContentRegionAvail();
				if (!virtualViewport.initialized)
				{
					Gfx::ViewportRef newGfxVirtualViewport = gfx.NewViewport();
					virtualViewport.gfxViewportRef = newGfxVirtualViewport;
					virtualViewport.width = (u32)newSize.x;
					virtualViewport.height = (u32)newSize.y;
					virtualViewport.renderWidth = (u32)newSize.x;
					virtualViewport.renderHeight = (u32)newSize.y;
					virtualViewport.initialized = true;
				}
				{
					// Manage the resizing of the viewport.
					if (newSize.x != virtualViewport.width || newSize.y != virtualViewport.height)
						virtualViewport.currentlyResizing = true;
					if (virtualViewport.currentlyResizing && newSize.x == virtualViewport.width && newSize.y == virtualViewport.height)
					{
						virtualViewport.renderWidth = (u32)newSize.x;
						virtualViewport.renderHeight = (u32)newSize.y;
						virtualViewport.currentlyResizing = false;
					}
					virtualViewport.width = (u32)newSize.x;
					virtualViewport.height = (u32)newSize.y;
				}

				DrawViewportWidget(virtualViewport, newSize, { editorData.cameras.data(), editorData.cameras.size() });
			}
			ImGui::End();

			// The window was closed this frame
			if (!windowIsOpen)
			{
				gfx.DeleteViewport(virtualViewport.gfxViewportRef.ViewportID());
				editorData.viewports.erase(editorData.viewports.begin() + i);
				i--;
			}
		}
	}
}

namespace DEngine::Editor
{
	static void SetImGuiStyle();
}

DEngine::Editor::EditorData DEngine::Editor::Initialize()
{
	SetImGuiStyle();

	return {};
}

void DEngine::Editor::RenderImGuiStuff(EditorData& editorData, Gfx::Data& gfx)
{
	ImGui::NewFrame();

	static bool testShowWindow = true;

	ImGui::ShowDemoWindow(nullptr);
	DrawViewportManager(editorData);
	DrawCameraManager(editorData);
	DrawVirtualViewports(editorData, gfx);


	ImGui::EndFrame();
	ImGui::Render();
}

static void DEngine::Editor::SetImGuiStyle()
{
	ImGuiStyle* style = &ImGui::GetStyle();

	ImVec4* colors = style->Colors;

	colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab].x = Math::Lerp(colors[ImGuiCol_Header].x, colors[ImGuiCol_TitleBgActive].x, 0.8f);
	colors[ImGuiCol_Tab].y = Math::Lerp(colors[ImGuiCol_Header].y, colors[ImGuiCol_TitleBgActive].y, 0.8f);
	colors[ImGuiCol_Tab].z = Math::Lerp(colors[ImGuiCol_Header].z, colors[ImGuiCol_TitleBgActive].z, 0.8f);
	colors[ImGuiCol_Tab].w = Math::Lerp(colors[ImGuiCol_Header].w, colors[ImGuiCol_TitleBgActive].w, 0.8f);
	colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
	colors[ImGuiCol_TabActive].x = Math::Lerp(colors[ImGuiCol_HeaderActive].x, colors[ImGuiCol_TitleBgActive].x, 0.60f);
	colors[ImGuiCol_TabActive].y = Math::Lerp(colors[ImGuiCol_HeaderActive].y, colors[ImGuiCol_TitleBgActive].y, 0.60f);
	colors[ImGuiCol_TabActive].z = Math::Lerp(colors[ImGuiCol_HeaderActive].z, colors[ImGuiCol_TitleBgActive].z, 0.60f);
	colors[ImGuiCol_TabActive].w = Math::Lerp(colors[ImGuiCol_HeaderActive].w, colors[ImGuiCol_TitleBgActive].w, 0.60f);
	colors[ImGuiCol_TabUnfocused].x = Math::Lerp(colors[ImGuiCol_Tab].x, colors[ImGuiCol_TitleBg].x, 0.80f);
	colors[ImGuiCol_TabUnfocused].y = Math::Lerp(colors[ImGuiCol_Tab].y, colors[ImGuiCol_TitleBg].y, 0.80f);
	colors[ImGuiCol_TabUnfocused].z = Math::Lerp(colors[ImGuiCol_Tab].z, colors[ImGuiCol_TitleBg].z, 0.80f);
	colors[ImGuiCol_TabUnfocused].w = Math::Lerp(colors[ImGuiCol_Tab].w, colors[ImGuiCol_TitleBg].w, 0.80f);
	colors[ImGuiCol_TabUnfocusedActive].x = Math::Lerp(colors[ImGuiCol_TabActive].x, colors[ImGuiCol_TitleBg].x, 0.40f);
	colors[ImGuiCol_TabUnfocusedActive].y = Math::Lerp(colors[ImGuiCol_TabActive].y, colors[ImGuiCol_TitleBg].y	, 0.40f);
	colors[ImGuiCol_TabUnfocusedActive].z = Math::Lerp(colors[ImGuiCol_TabActive].z, colors[ImGuiCol_TitleBg].z, 0.40f);
	colors[ImGuiCol_TabUnfocusedActive].w = Math::Lerp(colors[ImGuiCol_TabActive].w, colors[ImGuiCol_TitleBg].w, 0.40f);
	colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_HeaderActive];
	colors[ImGuiCol_DockingPreview].w *= 0.7f;
	colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}