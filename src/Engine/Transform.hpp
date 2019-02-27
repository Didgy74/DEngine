#pragma once

#include "Math/Vector/Vector.hpp"
#include "Math/Matrix/Matrix.hpp"
#include "Math/UnitQuaternion.hpp"

#include "Enum.hpp"

namespace Engine
{
	class SceneObject;

	class Transform
	{
	public:
		Math::Vector3D localPosition;
		Math::UnitQuaternion<> localRotation;
		Math::Vector3D scale;

		SceneObject& GetSceneObject() const;

		Math::Vector3D GetPosition(Space space) const;

		Math::Matrix<4, 3> GetModel_Reduced(Space space) const;
		Math::Matrix<3, 2> GetModel2D_Reduced(Space space) const;

		Math::Matrix2x2 GetRotationModel2D(Space space) const;

	private:
		Transform(SceneObject& owningSceneObject);

		SceneObject& sceneObject;

		friend class SceneObject;
	};
}
