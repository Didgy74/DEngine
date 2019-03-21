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
			sprite(Asset::Sprite::None),
			position{ 0, 0 },
			rotation(0),
			scale{ 1, 1 }
		{
		}

		SpriteRenderer::~SpriteRenderer()
		{
		}

		void SpriteRenderer::SetSprite(Asset::Sprite newSprite)
		{
			if (GetSprite() == newSprite)
				return;

			assert(Asset::CheckValid(newSprite));

			sprite = newSprite;
		}

		Asset::Sprite SpriteRenderer::GetSprite() const { return sprite; }

		Math::Matrix<3, 2> SpriteRenderer::GetModel2D_Reduced(Space space) const
		{
			const auto& scaleModel = Math::LinTran2D::Scale_Reduced(scale);
			const auto& rotateModel = Math::LinTran2D::Rotate_Reduced(rotation);
			auto localModel = Math::LinTran2D::Multiply(scaleModel, rotateModel);
			Math::LinTran2D::AddTranslation(localModel, position);

			if (space == Space::Local)
				return localModel;
			else
			{
				auto parentModel = GetSceneObject().transform.GetModel2D_Reduced(space);
				return Math::LinTran2D::Multiply(parentModel, localModel);
			}
		}

		Math::Matrix4x4 SpriteRenderer::GetModel(Space space) const
		{
			const auto& model = GetModel2D_Reduced(space);
			auto newMatrix = Math::Matrix4x4::Identity();
			for (size_t x = 0; x < model.GetWidth() - 1; x++)
			{
				for (size_t y = 0; y < model.GetHeight(); y++)
					newMatrix[x][y] = model[x][y];
			}

			for (size_t y = 0; y < model.GetHeight(); y++)
				newMatrix[3][y] = model[2][y];
			return newMatrix;
		}
	}
}


