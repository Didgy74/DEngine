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
			intensity(1)
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
				Multiply(GetSceneObject().transform.GetModel_Reduced(Space::World), localModel);
		}
	}
}