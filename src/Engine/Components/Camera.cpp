#include "DEngine/Components/Camera.hpp"

#include "DEngine/SceneObject.hpp"

#include "DMath/LinearTransform3D.hpp"

#include "DRenderer/Renderer.hpp"

#include <assert.h>

#include <iostream>

namespace Engine
{
	namespace Components
	{
		Camera::Camera(SceneObject& owningObject) :
			ParentType(owningObject)
		{
			//rotation = Math::UnitQuaternion<float>(Math::Vector3D{ 0, 1, 0 }, 180.f);
		}

		Camera::~Camera()
		{
		}

		void Camera::LookAt(const Math::Vector3D& newTarget, Space space)
		{
			
		}

		Math::Matrix<4, 3, float> Camera::GetModel_Reduced(Space space) const
		{
			using namespace Math::LinTran3D;
			auto localModel = Rotate_Reduced(rotation);
			AddTranslation(localModel, positionOffset);
			if (space == Space::Local)
				return localModel;
			else
				return Multiply_Reduced(GetSceneObject().GetModel_Reduced(Space::World), localModel);
		}

		Math::Matrix<4, 4, float> Camera::GetModel(Space space) const
		{
			return Math::LinTran3D::AsMat4(GetModel_Reduced(space));
		}

		Math::Matrix<4, 4, float> Camera::GetViewModel(Space space) const
		{
			using namespace Math::LinTran3D;

			auto model = Rotate_Reduced(rotation);
			AddTranslation(model, positionOffset);
			
			// Switch invert Z and X axis
			Math::Matrix<4, 3, float> testReduced{};
			for (size_t i = 0; i < 3; i++)
				testReduced.At(i, i) = 1;
			testReduced.At(2, 2) = -testReduced.At(2, 2);
			testReduced.At(0, 0) = -testReduced.At(0, 0);
			model = Multiply_Reduced(testReduced, model);
	
			if (space == Space::World)
				model = Multiply_Reduced(GetSceneObject().GetModel_Reduced(Space::World), model);

			auto modelMat4 = AsMat4(model);


			auto inverseOpt = modelMat4.GetInverse();
			assert(inverseOpt.has_value() && "Error. Couldn't find inverse matrix of camera.");
			auto inverse = inverseOpt.value();

			return inverse;
		}

		Math::Vector<3, float> Camera::GetPosition(Space space) const
		{
			using namespace Math::LinTran3D;

			auto localPos = positionOffset;
			if (space == Space::Local)
				return localPos;
			else if (space == Space::World)
				return Multiply_Reduced(GetSceneObject().GetModel_Reduced(space), localPos);
			else
			{
				assert(false && "Invalid enum value.");
				return {};
			}
		}
	}
}


