#include "Editor.hpp"
#include "DEngine/Time.hpp"
#include "DEngine/Math/LinearTransform3D.hpp"

#include "DEngine/Application.hpp"

#include "ImGui/imgui.h"

#include <iostream>
#include <DEngine/Application.hpp>

namespace DEngine::Editor
{
	static void DrawMainMenuBar(EditorData& editorData)
	{
		if (ImGui::BeginMainMenuBar())
		{
			editorData.mainMenuBarSize = ImGui::GetWindowSize();

			if (ImGui::MenuItem("File", nullptr, nullptr, true))
			{

			}

			if (ImGui::MenuItem("     ", nullptr, nullptr, false))
			{

			}

			ImGui::Separator();

			if (ImGui::MenuItem("Delta:", nullptr, nullptr, false))
			{

			}

			auto now = std::chrono::steady_clock::now();
			if (std::chrono::duration<float>(now - editorData.deltaTimePoint).count() >= editorData.deltaTimeRefreshTime)
			{
				editorData.displayedDeltaTime = std::to_string(Time::Delta());
				editorData.deltaTimePoint = now;
			}

			if (ImGui::MenuItem(editorData.displayedDeltaTime.c_str(), nullptr, nullptr, false))
			{

			}

			ImGui::EndMainMenuBar();
		}
	}

	static void DrawViewportManager(EditorData& editorData)
	{
		ImGui::SetNextWindowDockID(editorData.dockSpaceID, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints({ 250, 250 }, { 8192, 8192 });

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
		ImGui::SetNextWindowDockID(editorData.dockSpaceID, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints({ 250, 250 }, { 8192, 8192 });
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

			// DEBUG STUFF
			// Print all touch inputs

			auto inputArray = App::TouchInputs();
			for (auto& input : inputArray)
			{
				std::string str = std::string("ID: ") + std::to_string(input.id);
				str += std::string(", Event: ");



				ImGui::Text("%s", str.c_str());
			}
		}
		ImGui::End();
	}

	static void RenderVirtualViewport_MenuBar(
		Viewport& viewport,
		Std::Span<Std::Pair<uSize, Camera> const> cameras)
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
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
	}

	// This also handles camera controls
	static void InitializeVirtualViewport(
		Viewport& viewport,
		ImVec2 currentSize,
		Gfx::Data& gfx);
	static void HandleVirtualViewportResizing(
		Viewport& viewport,
		ImVec2 currentSize);
	static void HandleViewportCameraMovement(
		Camera& cam);
	static void DrawAllVirtualViewports(EditorData& editorData, Gfx::Data& gfx);
	static void DrawVirtualViewportContent(
		EditorData& editorData,
		Gfx::Data& gfx,
		uSize viewportID,
		Viewport& viewport)
	{
		viewport.visible = true;
		RenderVirtualViewport_MenuBar(viewport, { editorData.cameras.data(), editorData.cameras.size() });

		ImVec2 currentSize = ImGui::GetContentRegionAvail();
		if (!viewport.initialized)
			InitializeVirtualViewport(viewport, currentSize, gfx);
		HandleVirtualViewportResizing(viewport, currentSize);

		ImGui::Image(viewport.gfxViewportRef.ImGuiTexID(), currentSize);

		bool viewportIsHovered = ImGui::IsItemHovered();

		if (viewportIsHovered)
		{
			if (ImGui::GetIO().MouseDownDuration[0] > editorData.viewportFullscreenHoldTime)
			{
				editorData.insideFullscreenViewport = true;
				editorData.fullscreenViewportID = viewportID;
			}

			if (App::ButtonValue(App::Button::RightMouse))
			{
				Camera* cam = &viewport.camera;
				if (viewport.cameraID != Viewport::invalidCamID)
				{
					cam = nullptr;
					// Find the camera
					for (auto& [camID, camera] : editorData.cameras)
					{
						if (camID == viewport.cameraID)
						{
							cam = &camera;
							break;
						}
					}
					assert(cam != nullptr);
				}
#pragma warning( suppress : 6011 )
				HandleViewportCameraMovement(*cam);
			}
		}
	}
}

// This also handles camera controls
static void  DEngine::Editor::InitializeVirtualViewport(
	Viewport& viewport,
	ImVec2 currentSize,
	Gfx::Data& gfx)
{
	Gfx::ViewportRef newGfxVirtualViewport = gfx.NewViewport();
	viewport.gfxViewportRef = newGfxVirtualViewport;
	viewport.width = (u32)currentSize.x;
	viewport.height = (u32)currentSize.y;
	viewport.renderWidth = (u32)currentSize.x;
	viewport.renderHeight = (u32)currentSize.y;
	viewport.initialized = true;
}

static void DEngine::Editor::HandleVirtualViewportResizing(
	Viewport& viewport,
	ImVec2 currentSize)
{
	// Manage the resizing of the mainImGuiViewport.
	if (currentSize.x != viewport.width || currentSize.y != viewport.height)
		viewport.currentlyResizing = true;
	if (viewport.currentlyResizing && currentSize.x == viewport.width && currentSize.y == viewport.height)
	{
		viewport.renderWidth = (u32)currentSize.x;
		viewport.renderHeight = (u32)currentSize.y;
		viewport.currentlyResizing = false;
	}
	viewport.width = (u32)currentSize.x;
	viewport.height = (u32)currentSize.y;
}

static void DEngine::Editor::HandleViewportCameraMovement(
	Camera& cam)
{
	auto deltaOpt = App::Mouse();
	if (deltaOpt.HasValue())
	{
		auto delta = deltaOpt.Value();

		f32 sensitivity = 0.75f;
		i32 amountX = delta.delta[0];
		// Apply left and right rotation
		cam.rotation = Math::UnitQuat::FromVector(Math::Vec3D::Up(), -sensitivity * amountX) * cam.rotation;

		i32 amountY = delta.delta[1];

		// Limit rotation up and down
		Math::Vec3D forward = Math::LinTran3D::ForwardVector(cam.rotation);
		float dot = Math::Vec3D::Dot(forward, Math::Vec3D::Up());
		if (dot <= -0.99f)
			amountY = Math::Min(0, amountY);
		else if (dot >= 0.99f)
			amountY = Math::Max(0, amountY);

		// Apply up and down rotation
		Math::Vec3D right = Math::LinTran3D::RightVector(cam.rotation);
		cam.rotation = Math::UnitQuat::FromVector(right, sensitivity * amountY) * cam.rotation;

		// Handle camera movement
		f32 moveSpeed = 5.f;
		if (App::ButtonValue(App::Button::W))
			cam.position -= moveSpeed * Math::LinTran3D::ForwardVector(cam.rotation) * Time::Delta();
		if (App::ButtonValue(App::Button::S))
			cam.position += moveSpeed * Math::LinTran3D::ForwardVector(cam.rotation) * Time::Delta();
		if (App::ButtonValue(App::Button::D))
			cam.position += moveSpeed * Math::LinTran3D::RightVector(cam.rotation) * Time::Delta();
		if (App::ButtonValue(App::Button::A))
			cam.position -= moveSpeed * Math::LinTran3D::RightVector(cam.rotation) * Time::Delta();
		if (App::ButtonValue(App::Button::Space))
			cam.position.y -= moveSpeed * Time::Delta();
		if (App::ButtonValue(App::Button::LeftCtrl))
			cam.position.y += moveSpeed * Time::Delta();
	}
}

static void DEngine::Editor::DrawAllVirtualViewports(EditorData& editorData, Gfx::Data& gfx)
{
	// We use signed integer because of the way we remove the mainImGuiViewport.
	for (iSize i = 0; i < (iSize)editorData.viewports.size(); i += 1)
	{
		auto const& viewportID = editorData.viewports[i].a;
		auto& virtualViewport = editorData.viewports[i].b;

		// Check that our selected camera still exists.
		// If it no longer exists, fallback to the standard one.
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

		bool windowIsOpen = true;
		ImGui::SetNextWindowDockID(editorData.dockSpaceID, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize({ 250, 250 }, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints({ 200, 200 }, { 8192, 8192 });
		ImGuiWindowFlags windowFlags = 0;
		windowFlags |= ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse;
		windowFlags |= ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar;
		windowFlags |= ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar;
		std::string name = std::string("Viewport #") + std::to_string(viewportID);
		if (ImGui::Begin(name.data(), &windowIsOpen, windowFlags))
		{
			DrawVirtualViewportContent(
				editorData,
				gfx,
				viewportID,
				virtualViewport);
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

namespace DEngine::Editor
{
	static void SetImGuiStyle();
}

DEngine::Editor::EditorData DEngine::Editor::Initialize()
{
	EditorData returnVal{};

	SetImGuiStyle();

	returnVal.deltaTimePoint = std::chrono::steady_clock::now();
	returnVal.displayedDeltaTime = "0.016";


	return {};
}

void DEngine::Editor::RenderImGuiStuff(EditorData& editorData, Gfx::Data& gfx)
{
	ImGui::NewFrame();

	editorData.dockSpaceID = ImGui::GetID("test");

	DrawMainMenuBar(editorData);

	{
		ImGuiDockNodeFlags dockspaceFlags = 0;
		//dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		ImGuiViewport* mainImGuiViewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(mainImGuiViewport->Pos);
		ImGui::SetNextWindowSize(mainImGuiViewport->Size);
		ImGui::SetNextWindowViewport(mainImGuiViewport->ID);
		//ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		//ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background 
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		//bool test = false;
		ImGui::Begin("DockSpace Demo", nullptr, window_flags);
		//ImGui::PopStyleVar(3);

		ImGui::DockSpace(editorData.dockSpaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);
		
		ImGui::End();
	}

	
	static bool testShowWindow = true;
	if (testShowWindow)
	{
		ImGui::ShowDemoWindow(&testShowWindow);
	}


	if (!editorData.insideFullscreenViewport)
	{
		DrawViewportManager(editorData);
		DrawCameraManager(editorData);
		DrawAllVirtualViewports(editorData, gfx);
	}
	else
	{
		bool windowIsOpen = true;
		ImGuiViewport* mainImguiViewport = ImGui::GetMainViewport();

		ImVec2 windowSize = mainImguiViewport->Size;
		windowSize.y -= editorData.mainMenuBarSize.y;

		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
		ImGui::SetNextWindowPos({ 0, editorData.mainMenuBarSize.y }, ImGuiCond_Always);
		ImGui::SetNextWindowFocus();
		ImGuiWindowFlags windowFlags = 0;
		windowFlags |= ImGuiWindowFlags_NoCollapse;
		windowFlags |= ImGuiWindowFlags_NoScrollbar;
		windowFlags |= ImGuiWindowFlags_NoResize;
		windowFlags |= ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoTitleBar;
		if (ImGui::Begin("Testtestest", &windowIsOpen, windowFlags))
		{
			uSize viewportIndex = static_cast<uSize>(-1);
			for (uSize i = 0; i < editorData.viewports.size(); i += 1)
			{
				if (editorData.viewports[i].a == editorData.fullscreenViewportID)
				{
					viewportIndex = i;
					break;
				}
			}
			Viewport& vp = editorData.viewports[viewportIndex].b;
			vp.visible = true;


			ImVec2 imgSize = ImGui::GetContentRegionAvail();

			HandleVirtualViewportResizing(vp, imgSize);
			ImVec2 imgPos = ImGui::GetCursorPos();
			ImGui::Image(vp.gfxViewportRef.ImGuiTexID(), imgSize);

			uSize camIndex = static_cast<uSize>(-1);
			for (uSize i = 0; i < editorData.cameras.size(); i += 1)
			{
				if (vp.cameraID == editorData.cameras[i].a)
				{
					camIndex = i;
					break;
				}
			}
			Camera& cam = camIndex == static_cast<uSize>(-1) ? vp.camera : editorData.cameras[camIndex].b;

			// Handle mouse FreeLook
			if (ImGui::IsItemHovered())
			{
				if (App::ButtonValue(App::Button::RightMouse))
					HandleViewportCameraMovement(cam);
			}


		}
		ImGui::End();

		if (App::ButtonEvent(App::Button::Escape) == App::KeyEventType::Pressed ||
			App::ButtonEvent(App::Button::Back) == App::KeyEventType::Pressed)
		{
			editorData.insideFullscreenViewport = false;
			editorData.fullscreenViewportID = EditorData::invalidViewportID;
		}
	}


	ImGui::EndFrame();
	ImGui::Render();
}

static void DEngine::Editor::SetImGuiStyle()
{
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 4.f;
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