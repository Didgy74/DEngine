#include "AssManRendererConnect.hpp"

#include "DEngine/AssetManager/AssetManager.hpp"

#include <cassert>

#include <iostream>

namespace Engine
{
	Renderer::CameraInfo GetRendererCameraInfo(const Components::Camera& in)
	{
		Renderer::CameraInfo cameraInfo{};

		cameraInfo.fovY = in.fov;
		cameraInfo.orthoWidth = in.orthographicWidth;
		cameraInfo.zNear = in.zNear;
		cameraInfo.zFar = in.zFar;

		cameraInfo.transform = in.GetViewModel(Space::World);

		cameraInfo.worldSpacePos = in.GetPosition(Space::World);

		switch (in.projectionMode)
		{
		case Components::Camera::ProjectionMode::Perspective:
			cameraInfo.projectMode = Renderer::CameraInfo::ProjectionMode::Perspective;
			break;
		case Components::Camera::ProjectionMode::Orthgraphic:
			cameraInfo.projectMode = Renderer::CameraInfo::ProjectionMode::Orthographic;
			break;
		default:
			assert(false && "Invalid Camera Projection Mode.");
		}

		return cameraInfo;
	}

	std::optional<DTex::TexDoc> LoadTexture(size_t i)
	{
		auto path = AssMan::GetTextureInfo(i);
		if (path.path == "")
			return {};

		auto loadResult = DTex::LoadFromFile(path.path);
		if (loadResult.GetResultInfo() != DTex::ResultInfo::Success)
			return {};

		return std::optional<DTex::TexDoc>{ std::move(loadResult.GetValue()) };
	}
}
