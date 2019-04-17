#include "DEngine/Components/SpriteRenderer.hpp"

#include <array>
#include <cassert>

#include "DEngine/Scene.hpp"
#include "DEngine/SceneObject.hpp"

#include "DMath/LinearTransform2D.hpp"

namespace Engine
{
	namespace Components
	{
		SpriteRenderer::SpriteRenderer(SceneObject& owningObject) :
			ParentType(owningObject)
		{
		}

		SpriteRenderer::~SpriteRenderer()
		{
		}

		void SpriteRenderer::SetSprite(size_t newSprite)
		{
			if (GetSprite() == newSprite)
				return;

			sprite = newSprite;
		}

		size_t SpriteRenderer::GetSprite() const { return sprite; }

		Math::Matrix<3, 2> SpriteRenderer::GetModel2D_Reduced(Space space) const
		{
			using namespace Math::LinTran2D;

			const auto& scaleModel = Scale_Reduced(scale);
			const auto& rotateModel = Rotate_Reduced(rotation);
			auto localModel = Multiply_Reduced(scaleModel, rotateModel);
			AddTranslation(localModel, positionOffset);

			if (space == Space::Local)
				return localModel;
			else
			{
				auto parentModel = GetSceneObject().GetModel2D_Reduced(space);
				return Multiply_Reduced(parentModel, localModel);
			}
		}

		Math::Matrix4x4 SpriteRenderer::GetModel(Space space) const
		{
			const auto& model = GetModel2D_Reduced(space);
			return Math::LinTran2D::AsMat4(model);
		}
	}
}


