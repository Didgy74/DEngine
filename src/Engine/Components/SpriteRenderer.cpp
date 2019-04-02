#include "SpriteRenderer.hpp"

#include <array>
#include <cassert>

#include "../Scene.hpp"
#include "../SceneObject.hpp"

#include "DMath/LinearTransform2D.hpp"

namespace Engine
{
	namespace Components
	{
		SpriteRenderer::SpriteRenderer(SceneObject& owningObject) :
			ParentType(owningObject),
			sprite(AssMan::Sprite::None),
			positionOffset{ 0, 0 },
			rotation(0),
			scale{ 1, 1 }
		{
		}

		SpriteRenderer::~SpriteRenderer()
		{
		}

		void SpriteRenderer::SetSprite(AssMan::Sprite newSprite)
		{
			if (GetSprite() == newSprite)
				return;

			sprite = newSprite;
		}

		AssMan::Sprite SpriteRenderer::GetSprite() const { return sprite; }

		Math::Matrix<3, 2> SpriteRenderer::GetModel2D_Reduced(Space space) const
		{
			using namespace Math::LinTran2D;

			const auto& scaleModel = Scale_Reduced(scale);
			const auto& rotateModel = Rotate_Reduced(rotation);
			auto localModel = Multiply(scaleModel, rotateModel);
			AddTranslation(localModel, positionOffset);

			if (space == Space::Local)
				return localModel;
			else
			{
				auto parentModel = GetSceneObject().transform.GetModel2D_Reduced(space);
				return Multiply(parentModel, localModel);
			}
		}

		Math::Matrix4x4 SpriteRenderer::GetModel(Space space) const
		{
			const auto& model = GetModel2D_Reduced(space);
			return Math::LinTran2D::AsMat4(model);
		}
	}
}


