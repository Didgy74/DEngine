#include "Editor.hpp"
#include "DEngine/InputRaw.hpp"
#include "DEngine/Time.hpp"
#include "DEngine/Math/LinearTransform3D.hpp"

#include "ImGui/imgui.h"

namespace DEngine::Editor
{
	void DrawViewportManager(EditorData& editorData)
	{
		if (ImGui::Begin("Testerrr", nullptr))
		{
			if (ImGui::Button("New viewport!"))
			{
				// Add a new camera to our array. Figure out which is the next
				// ID we can use and which index to insert into.
				// Or just append if we found no available index
				u8 availableID = 255;
				u8 insertionIndex = 255;
				for (uSize i = 1; i < editorData.viewports.size(); i += 1)
				{
					if (editorData.viewports[i - 1].id < editorData.viewports[i].id - 1)
					{
						availableID = editorData.viewports[i - 1].id + 1;
						insertionIndex = i;
						break;
					}
				}

				if (insertionIndex == 255)
				{
					EditorData::Viewport newViewport{};
					newViewport.id = (u8)editorData.viewports.size();
					editorData.viewports.push_back(newViewport);
				}
				else
				{
					EditorData::Viewport newViewport{};
					newViewport.id = availableID;
					editorData.viewports.insert(editorData.viewports.begin() + insertionIndex, newViewport);
				}
			}
		}
		ImGui::End();
	}

	void DrawCameraManager(EditorData& editorData)
	{
		ImGui::SetNextWindowSize({ 250, 250 }, ImGuiCond_FirstUseEver);
		ImGuiWindowFlags windowFlags = 0;
		windowFlags |= ImGuiWindowFlags_::ImGuiWindowFlags_NoResize;
		if (ImGui::Begin("List of cameras"))
		{
			// Draw Camera info widgets
			for (auto const& camera : editorData.cameras)
			{
				std::string cameraName = std::string("Camera #") + std::to_string(camera.id);
				ImGui::Text("%s", cameraName.data());
			}

			ImVec2 size = ImGui::GetContentRegionAvail();
			if (ImGui::Button("+", { size.x, 0 }))
			{
				// Add a new camera to our array. Figure out which is the next
				// ID we can use and which index to insert into.
				// Or just append if we found no available index
				u8 availableID = 255;
				u8 insertionIndex = 255;
				for (uSize i = 1; i < editorData.cameras.size(); i += 1)
				{
					if (editorData.cameras[i - 1].id < editorData.cameras[i].id - 1)
					{
						availableID = editorData.cameras[i - 1].id + 1;
						insertionIndex = i;
						break;
					}
				}

				if (insertionIndex == 255)
				{
					Camera newCam{};
					newCam.id = (u8)editorData.cameras.size();
					editorData.cameras.push_back(newCam);
				}
				else
				{
					Camera newCam{};
					newCam.id = availableID;
					editorData.cameras.insert(editorData.cameras.begin() + insertionIndex, newCam);
				}
			}
		}
		ImGui::End();
	}

	void RenderVirtualViewport_MenuBar(
		EditorData::Viewport& viewport,
		Cont::Span<Camera const> cameras)
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Camera"))
			{
				bool yo = false;
				if (viewport.cameraID == 255)
					yo = true;
				if (ImGui::MenuItem("Free", nullptr, yo));
				{
					viewport.cameraID = 255;
				}

				ImGui::Separator();
				ImGui::MenuItem("Scene #0", nullptr, false, false);


				for (auto const& camera : cameras)
				{
					bool yo = false;
					if (viewport.cameraID == camera.id)
						yo = true;
					std::string cameraString = std::string("    Camera #") + std::to_string(camera.id);
					if (ImGui::MenuItem(cameraString.data(), nullptr, yo))
					{
						viewport.cameraID = camera.id;
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}

	void DrawVirtualViewports(EditorData& editorData, Gfx::Data& gfx)
	{
		for (auto& virtualViewport : editorData.viewports)
		{
			std::string name = std::string("Viewport #") + std::to_string(virtualViewport.id);

			ImGui::SetNextWindowSize({ 250, 250 }, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints({ 100, 100 }, { 8192, 8192 });
			ImGuiWindowFlags windowFlags = 0;
			windowFlags |= ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse;
			windowFlags |= ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar;
			windowFlags |= ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar;
			if (ImGui::Begin(name.data(), nullptr, windowFlags))
			{
				RenderVirtualViewport_MenuBar(virtualViewport, { editorData.cameras.data(), editorData.cameras.size() });



				ImVec2 newSize = ImGui::GetContentRegionAvail();
				newSize.x -= 25;
				newSize.y -= 25;
				if (!virtualViewport.initialized)
				{
					Gfx::ViewportRef newGfxVirtualViewport = gfx.NewViewport(virtualViewport.id);
					virtualViewport.imguiTextureID = newGfxVirtualViewport.ImGuiTexID();
					virtualViewport.width = (u32)newSize.x;
					virtualViewport.height = (u32)newSize.y;
					virtualViewport.renderWidth = (u32)newSize.x;
					virtualViewport.renderHeight = (u32)newSize.y;
					virtualViewport.initialized = true;
				}

				virtualViewport.visible = true;

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

				ImVec2 imagePos = ImGui::GetCursorScreenPos();

				ImGui::Image(virtualViewport.imguiTextureID, newSize);


				if (ImGui::IsItemHovered() && Input::Raw::GetValue(Input::Raw::Button::RightMouse))
				{
					Camera& cam = virtualViewport.cameraID != 255 ? editorData.cameras[virtualViewport.cameraID] : virtualViewport.camera;
					auto delta = Input::Raw::GetMouseDelta();

					f32 sensitivity = 0.1f;
					f32 amountX = Input::Raw::GetMouseDelta()[0];

					// Apply left and right rotation
					cam.rotation = Math::UnitQuat::FromVector(Math::Vec3D::Up(), -sensitivity * amountX) * cam.rotation;

					auto amountY = Input::Raw::GetMouseDelta()[1];

					// Limit rotation up and down
					Math::Vec3D forward = Math::LinTran3D::ForwardVector(cam.rotation);
					float dot = Math::Vec3D::Dot(forward, Math::Vec3D::Up());
					if (dot <= -0.99f)
						amountY = Math::Max((i16)0, amountY);
					else if (dot >= 0.99f)
						amountY = Math::Min((i16)0, amountY);

					// Apply up and down rotation
					Math::Vec3D right = Math::LinTran3D::RightVector(cam.rotation);
					cam.rotation = Math::UnitQuat::FromVector(right, sensitivity * amountY) * cam.rotation;

					// Handle camera movement
					f32 moveSpeed = 5.f;
					if (Input::Raw::GetValue(Input::Raw::Button::W))
						cam.position -= moveSpeed * Math::LinTran3D::ForwardVector(cam.rotation) * Time::Delta();
					if (Input::Raw::GetValue(Input::Raw::Button::S))
						cam.position += moveSpeed * Math::LinTran3D::ForwardVector(cam.rotation) * Time::Delta();
					if (Input::Raw::GetValue(Input::Raw::Button::D))
						cam.position += moveSpeed * Math::LinTran3D::RightVector(cam.rotation) * Time::Delta();
					if (Input::Raw::GetValue(Input::Raw::Button::A))
						cam.position -= moveSpeed * Math::LinTran3D::RightVector(cam.rotation) * Time::Delta();
					if (Input::Raw::GetValue(Input::Raw::Button::Space))
						cam.position.y -= moveSpeed * Time::Delta();
					if (Input::Raw::GetValue(Input::Raw::Button::LeftCtrl))
						cam.position.y += moveSpeed * Time::Delta();
				}
			}
			else
			{
				virtualViewport.visible = false;
			}
			ImGui::End();
		}
	}
}

void DEngine::Editor::RenderImGuiStuff(EditorData& editorData, Gfx::Data& gfx)
{
	if (Time::TickCount() == 1)
	{
		editorData.cameras.resize(5);
		for (int i = 0; i < editorData.cameras.size(); i++)
			editorData.cameras[i].id = i;
	}

	ImGui::NewFrame();

	static bool testShowWindow = true;
	ImGui::ShowDemoWindow(nullptr);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7.5f, 7.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	DrawViewportManager(editorData);
	DrawCameraManager(editorData);
	DrawVirtualViewports(editorData, gfx);


	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();


	ImGui::EndFrame();
	ImGui::Render();
}