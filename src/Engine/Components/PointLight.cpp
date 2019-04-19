#include "DEngine/Components/PointLight.hpp"

#include "DEngine/SceneObject.hpp"

#include "DMath/LinearTransform3D.hpp"

namespace Engine
{
	namespace Components
	{
		PointLight::PointLight(SceneObject& owningObject) :
			ParentType(owningObject)
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
				return Multiply_Reduced(GetSceneObject().GetModel_Reduced(Space::World), localModel);
		}

		Math::Matrix4x4 PointLight::GetModel(Space space) const
		{
			using namespace Math::LinTran3D;
			return AsMat4(GetModel_Reduced(space));
		}

		Math::Vector<3, float> PointLight::GetPosition(Space space) const
		{
			return Math::LinTran3D::GetTranslation(GetModel_Reduced(space));
		}
	}
}