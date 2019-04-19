#include "InputRotate.hpp"

#include "DEngine/SceneObject.hpp"
#include "DEngine/Scene.hpp"

#include "DMath/Trigonometric.hpp"

#include "DEngine/Input/InputRaw.hpp"

namespace Assignment02
{
	InputRotate::InputRotate(Engine::SceneObject& owner) :
		ParentType(owner)
	{

	}

	void InputRotate::SceneStart()
	{
		ParentType::SceneStart();
	}

	void InputRotate::Tick()
	{
		ParentType::Tick();

		const auto& scene = GetSceneObject().GetScene();
		float deltaTime = scene.GetTimeData().GetDeltaTime();

		if (Engine::Input::Raw::GetValue(Engine::Input::Raw::Button::Up))
			GetSceneObject().localRotation = Math::UnitQuaternion<float>({ 0.f, 0.f, 10.f * deltaTime }) * GetSceneObject().localRotation;
		if (Engine::Input::Raw::GetValue(Engine::Input::Raw::Button::Down))
			GetSceneObject().localRotation = Math::UnitQuaternion<float>({ 0.f, 0.f, -10.f * deltaTime }) * GetSceneObject().localRotation;
	}
}