#include "Transform.hpp"

#include "Math/LinearTransform3D.hpp"
#include "Math/LinearTransform2D.hpp"

#include "Scene.hpp"
#include "SceneObject.hpp"

Engine::Transform::Transform(SceneObject& owningSceneObject) :
	sceneObject(owningSceneObject),
	localPosition(),
	localRotation(),
	scale(Math::Vector3D::One())
{
}

Engine::SceneObject& Engine::Transform::GetSceneObject() const { return sceneObject; }

Math::Vector3D Engine::Transform::GetPosition(Space space) const
{
	if (space == Space::Local)
		return localPosition;
	else
	{
		if (GetSceneObject().GetParent() == nullptr)
			return localPosition;
		else
		{
			assert(false);
			return {};
		}
	}
}

Math::Matrix<4, 3> Engine::Transform::GetModel_Reduced(Space space) const
{
	const auto& scaleModel = Math::LinTran3D::Scale_Reduced(scale);
	const auto& rotateModel = Math::LinTran3D::Rotate_Reduced(localRotation);
	auto localModel = Math::LinTran3D::Multiply(scaleModel, rotateModel);
	Math::LinTran3D::AddTranslation(localModel, localPosition);
	return localModel;
}

Math::Matrix<3, 2> Engine::Transform::GetModel2D_Reduced(Space space) const
{
	const auto& scaleModel = Math::LinTran2D::Scale_Reduced(scale.x, scale.y);
	const auto& rotate = Math::LinTran2D::Rotate_Reduced(localRotation);
	auto localModel = Math::LinTran2D::Multiply(scaleModel, rotate);
	Math::LinTran2D::AddTranslation(localModel, localPosition.x, localPosition.y);
	return localModel;
}

Math::Matrix2x2 Engine::Transform::GetRotationModel2D(Engine::Space space) const
{
	return Math::Matrix2x2();
}
