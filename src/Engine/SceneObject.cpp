#include "DEngine/SceneObject.hpp"

#include "Engine.hpp"

#include "DMath/LinearTransform2D.hpp"
#include "DMath/LinearTransform3D.hpp"

Engine::SceneObject::SceneObject(Scene& owningScene, size_t indexInScene) : 
	sceneRef(owningScene)
{
}

Engine::SceneObject::~SceneObject()
{
	
}

Engine::Scene& Engine::SceneObject::GetScene() 
{
	return sceneRef.get();
}

Engine::SceneObject* Engine::SceneObject::GetParent() const { return parent; }

Math::Vector3D Engine::SceneObject::GetPosition(Space space) const
{
	if (space == Space::Local)
		return localPosition;
	else
	{
		if (parent == nullptr)
			return localPosition;
		else
		{
			assert(false);
			return {};
		}
	}
}

Math::Matrix<4, 3> Engine::SceneObject::GetModel_Reduced(Space space) const
{
	const auto& scaleModel = Math::LinTran3D::Scale_Reduced(localScale);
	const auto& rotateModel = Math::LinTran3D::Rotate_Reduced(localRotation);
	auto localModel = Math::LinTran3D::Multiply_Reduced(scaleModel, rotateModel);
	Math::LinTran3D::AddTranslation(localModel, localPosition);
	return localModel;
}

Math::Matrix<3, 2> Engine::SceneObject::GetModel2D_Reduced(Space space) const
{
	const auto& scaleModel = Math::LinTran2D::Scale_Reduced(localScale.x, localScale.y);
	const auto& rotate = Math::LinTran2D::Rotate_Reduced(localRotation);
	auto localModel = Math::LinTran2D::Multiply_Reduced(scaleModel, rotate);
	Math::LinTran2D::AddTranslation(localModel, localPosition.x, localPosition.y);
	return localModel;
}

Math::Matrix2x2 Engine::SceneObject::GetRotationModel2D(Space space) const
{
	return Math::Matrix2x2();
}

Math::Matrix<3, 3, float> Engine::SceneObject::GetRotationModel(Space space) const
{
	using namespace Math::LinearTransform3D;

	auto localModel = Rotate(localRotation);
	if (space == Space::Local || parent == nullptr)
		return localModel;
	else
		return parent->GetRotationModel(space) * localModel;
}

Math::Vector<3, float> Engine::SceneObject::GetForwardVector(Space space) const
{
	Math::UnitQuaternion<float> quat = localRotation;

	const auto & s = quat.GetS();
	const auto & x = quat.GetX();
	const auto & y = quat.GetY();
	const auto & z = quat.GetZ();

	return
	Math::Vector<3, float>
	{
		2 * x* z + 2 * y * s, 
		2 * y* z - 2 * x * s, 
		1 - 2 * Math::Sqrd(x) - 2 * Math::Sqrd(y)
	};
}

Math::Vector<3, float> Engine::SceneObject::GetUpVector(Space space) const
{
	Math::UnitQuaternion<float> quat = localRotation;

	const auto & s = quat.GetS();
	const auto & x = quat.GetX();
	const auto & y = quat.GetY();
	const auto & z = quat.GetZ();

	return
	Math::Vector<3, float>
	{
		2 * x* y - 2 * z * s,
		1 - 2 * Math::Sqrd(x) - 2 * Math::Sqrd(z), 
		2 * y* z + 2 * x * s
	};
}

Math::Vector<3, float> Engine::SceneObject::GetRightVector(Space space) const
{
	Math::UnitQuaternion<float> quat = localRotation;

	const auto& s = quat.GetS();
	const auto& x = quat.GetX();
	const auto& y = quat.GetY();
	const auto& z = quat.GetZ();

	return 
	Math::Vector<3, float>
	{ 
		1 - 2 * Math::Sqrd(y) - 2 * Math::Sqrd(z), 
		2 * x * y + 2 * z * s,
		2 * x * z - 2 * y * s 
	};
}

void Engine::Destroy(SceneObject& sceneObject)
{
	Core::Destroy(sceneObject);
}