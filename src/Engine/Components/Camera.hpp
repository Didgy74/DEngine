#pragma once

#include "ComponentBase.hpp"

#include "Math/Vector/Vector.hpp"

#include "../Renderer/Renderer.hpp"

namespace Engine
{
	class Camera : public ComponentBase
	{
	public:
		using ParentType = ComponentBase;

		enum class ProjectionMode
		{
			Perspective,
			Orthgraphic
		};

		static constexpr float defaultFovY = 50.f;
		static constexpr float defaultOrtographicWidth = 10.f;
		static constexpr float defaultZNear = 0.1f;
		static constexpr float defaultZFar = 100.f;
		static constexpr ProjectionMode defaultProjectionMode = ProjectionMode::Perspective;

		Camera(SceneObject& owningObject, size_t indexInSceneObject, size_t indexInScene) noexcept;
		~Camera();

		Math::Vector3D position;
		Math::Vector3D forward;
		Math::Vector3D up;

		float fov;
		float orthographicWidth;
		float zNear;
		float zFar;
		ProjectionMode activeProjectionMode;
		
		void LookAt(const Math::Vector3D& newTarget);

		Renderer::CameraInfo GetCameraInfo() const;
	};
}