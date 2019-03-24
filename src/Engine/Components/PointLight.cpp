#include "PointLight.hpp"

#include "../SceneObject.hpp"

#include "DMath/LinearTransform3D.hpp"

namespace Engine
{
	namespace Components
	{
		PointLight::PointLight(SceneObject& owningObject) :
			ParentType(owningObject),
			positionOffset(),
			intensity(1),
			color{1.f, 1.f, 1.f}
		{
		}

		PointLight::~PointLight()
		{
		}

		Math::Matrix<4, 3> PointLight::GetModel_Reduced(Space space) const
		{
			using namespace Math::LinTran3D;
			auto localModel = Translate_Reduced(positionOffset);
			
			if (space == Space::Local)
				return localModel;
			else
				return Multiply(GetSceneObject().transform.GetModel_Reduced(Space::World), localModel);
		}

		Math::Matrix4x4 PointLight::GetModel(Space space) const
		{
			using namespace Math::LinTran3D;
			return AsMat4(GetModel_Reduced(space));
		}
	}
}