#include "DEngine/Components/FreeLook.hpp"

#include "DMath/Vector/Vector.hpp"

#include "DEngine/Input/InputRaw.hpp"

#include "DEngine/SceneObject.hpp"
#include "DEngine/Scene.hpp"

namespace Engine
{
	namespace Components
	{
		FreeLook::FreeLook(SceneObject& owningObject) :
			ParentType(owningObject)
		{

		}

		void FreeLook::SceneStart()
		{
			ParentType::SceneStart();
		}

		void FreeLook::Tick()
		{
			ParentType::Tick();

			SceneObject& obj = GetSceneObject();
			const Scene& scene = obj.GetScene();
			float deltaTime = scene.GetTimeData().GetDeltaTime();

			if (Input::Raw::GetValue(Input::Raw::Button::RightMouse))
			{
				float sensitivity = 15;

				float amountX = Input::Raw::GetMouseDelta()[0];

				obj.localRotation = Math::UnitQuaternion<float>(Math::Vector3D{ 0, -sensitivity * deltaTime * amountX, 0 }) * obj.localRotation;

				auto amountY = -Input::Raw::GetMouseDelta()[1];

				Math::Vector3D forward = GetSceneObject().GetForwardVector(Space::Local);
				float dot = Math::Vector3D::Dot(forward, Math::Vector3D::Up());
				if (dot <= -0.99f)
					amountY = Math::Max(0, amountY);
				else if (dot >= 0.99f)
					amountY = Math::Min(0, amountY);

				Math::Vector3D right = GetSceneObject().GetRightVector(Space::Local);

				obj.localRotation = Math::UnitQuaternion<float>(right, sensitivity * deltaTime * -amountY) * obj.localRotation;
			}

			auto mat = GetSceneObject().GetRotationModel(Space::Local);
			
			Math::Vector3D right{ mat[0][0], mat[0][1], mat[0][2] };
			Math::Vector3D up{ mat[1][0], mat[1][1], mat[1][2] };
			Math::Vector3D forward{ mat[2][0], mat[2][1], mat[2][2] };
			

			// Handles origin movement for camera
			constexpr float speed = 5.f;

			if (Input::Raw::GetValue(Input::Raw::Button::A))
				obj.localPosition += right * speed * deltaTime;
			if (Input::Raw::GetValue(Input::Raw::Button::D))
				obj.localPosition -= right * speed * deltaTime;
			if (Input::Raw::GetValue(Input::Raw::Button::W))
				obj.localPosition += forward * speed * deltaTime;
			if (Input::Raw::GetValue(Input::Raw::Button::S))
				obj.localPosition -= forward * speed * deltaTime;
			if (Input::Raw::GetValue(Input::Raw::Button::Space))
				obj.localPosition += Math::Vector3D::Up() * speed * deltaTime;
			if (Input::Raw::GetValue(Input::Raw::Button::LeftCtrl))
				obj.localPosition += Math::Vector3D::Down() * speed * deltaTime;
		}
	}
}