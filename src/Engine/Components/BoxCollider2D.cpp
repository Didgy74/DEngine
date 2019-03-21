#include "BoxCollider2D.hpp"

#include "DMath/LinearTransform2D.hpp"
#include "../SceneObject.hpp"

#include <cassert>
#include <iostream>

namespace Engine
{
	namespace Components
	{
		BoxCollider2D::BoxCollider2D(SceneObject& owningObject) :
			ParentType(owningObject),
			size{ 1.f, 1.f },
			position{ 0, 0 },
			rotation{ 0 }
		{
		}

		BoxCollider2D::~BoxCollider2D()
		{
		}

		Math::Matrix3x3 BoxCollider2D::GetModel2D(Space space) const
		{
			auto newModel = Math::Matrix3x3::Identity();
			const auto& model = GetModel2D_Reduced(space);
			for (size_t x = 0; x < newModel.GetWidth(); x++)
			{
				for (size_t y = 0; y < newModel.GetHeight(); y++)
					newModel[x][y] = model[x][y];
			}
			return newModel;
		}

		Math::Matrix<3, 2> BoxCollider2D::GetModel2D_Reduced(Space space) const
		{
			const auto& scaleModel = Math::LinTran2D::Scale_Reduced(size);
			const auto& rotateModel = Math::LinTran2D::Rotate_Reduced<Math::AngleUnit::Degrees>(rotation);
			auto localModel = Math::LinTran2D::Multiply(scaleModel, rotateModel);
			Math::LinTran2D::AddTranslation(localModel, position);

			if (space == Space::Local)
				return localModel;
			else
			{
				const auto& parentModel = GetSceneObject().transform.GetModel2D_Reduced(space);
				return Math::LinTran2D::Multiply(parentModel, localModel);
			}
		}

		Math::Matrix2x2 BoxCollider2D::GetRotationModel2D(Space space) const
		{
			assert(space != Space::Local);

			auto localModel = Math::LinTran2D::Rotate(rotation);
			if (space == Space::Local)
				return localModel;
			else
				return GetSceneObject().transform.GetRotationModel2D(Space::World) * localModel;
		}

		Math::Matrix3x3 BoxCollider2D::GetRotationModel2D_Homo(Space space) const
		{
			assert(space != Space::Local);

			auto test = Math::LinTran2D::Rotate_Homo(rotation);
			return test;
		}

	}
}

