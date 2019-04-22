#include "DEngine/SceneObject.hpp"

#include "Engine.hpp"

#include "DMath/LinearTransform2D.hpp"
#include "DMath/LinearTransform3D.hpp"

namespace Engine
{
	SceneObject::SceneObject(Scene& owningScene, size_t indexInScene) :
		sceneRef(owningScene)
	{
	}

	SceneObject::~SceneObject()
	{

	}

	Scene& SceneObject::GetScene()
	{
		return sceneRef.get();
	}

	SceneObject* SceneObject::GetParent() const { return parent; }

	void SceneObject::SetParent(SceneObject* newParent)
	{
		if (parent == newParent)
			return;

		// Clean up old parent
		if (parent != nullptr)
		{
			auto& parentChildVector = parent->children;
			for (size_t i = 0; i < parentChildVector.size(); i++)
			{
				if (this == &parentChildVector[i].get())
				{
					parentChildVector[i] = parentChildVector.back();
					parentChildVector.pop_back();
					break;
				}
			}
		}

		parent = newParent;

		// Add reference to new parent
		if (parent != nullptr)
			parent->children.emplace_back(*this);
	}

	Math::Vector3D SceneObject::GetPosition(Space space) const
	{
		using namespace Math::LinTran3D;

		if (space == Space::Local)
			return localPosition;
		else
		{
			if (parent == nullptr)
				return localPosition;
			else
				return Multiply_Reduced(parent->GetModel_Reduced(space), localPosition);
		}
	}

	Math::Matrix<4, 3> SceneObject::GetModel_Reduced(Space space) const
	{
		using namespace Math::LinTran3D;

		auto localModel = Multiply_Reduced(Rotate_Reduced(localRotation), Scale_Reduced(localScale));
		Math::LinTran3D::AddTranslation(localModel, localPosition);

		if (space == Space::Local || parent == nullptr)
			return localModel;
		else if (space == Space::World)
			return Multiply_Reduced(parent->GetModel_Reduced(space), localModel);
		else
		{
			assert(false && "Error. Undefined enum value.");
			return {};
		}
	}

	Math::Matrix<3, 2> SceneObject::GetModel2D_Reduced(Space space) const
	{
		using namespace Math::LinTran2D;

		auto localModel = Multiply_Reduced(Scale_Reduced(localScale.AsVec2()), Rotate_Reduced(localRotation));
		AddTranslation(localModel, localPosition.AsVec2());

		if (space == Space::Local || parent == nullptr)
			return localModel;
		else if (space == Space::World)
			return Multiply_Reduced(parent->GetModel2D_Reduced(space), localModel);
		else
		{
			assert(false && "Error. Undefined enum value.");
			return {};
		}
	}

	Math::Matrix2x2 SceneObject::GetRotationModel2D(Space space) const
	{
		return Math::Matrix2x2();
	}

	Math::Matrix<3, 3, float> SceneObject::GetRotationModel(Space space) const
	{
		using namespace Math::LinTran3D;

		auto localModel = Rotate(localRotation);
		if (space == Space::Local || parent == nullptr)
			return localModel;
		else if (space == Space::World)
			return parent->GetRotationModel(space) * localModel;
		else
		{
			assert(false && "Error. Undefined enum value.");
			return {};
		}
	}

	Math::Vector<3, float> SceneObject::GetForwardVector(Space space) const
	{
		const auto& s = localRotation.GetS();
		const auto& x = localRotation.GetX();
		const auto& y = localRotation.GetY();
		const auto& z = localRotation.GetZ();

		Math::Vector<3, float> localForwardVector
		{
			2 * x * z + 2 * y * s,
			2 * y * z - 2 * x * s,
			1 - 2 * Math::Sqrd(x) - 2 * Math::Sqrd(y)
		};

		if (space == Space::Local || parent == nullptr)
			return localForwardVector;
		else if (space == Space::World)
			return parent->GetRotationModel(space) * localForwardVector;
		else
		{
			assert(false && "Error. Undefined enum value.");
			return {};
		}
	}

	Math::Vector<3, float> SceneObject::GetUpVector(Space space) const
	{
		const auto& s = localRotation.GetS();
		const auto& x = localRotation.GetX();
		const auto& y = localRotation.GetY();
		const auto& z = localRotation.GetZ();

		Math::Vector<3, float> localUpVector
		{
			2 * x * y - 2 * z * s,
			1 - 2 * Math::Sqrd(x) - 2 * Math::Sqrd(z),
			2 * y * z + 2 * x * s
		};

		if (space == Space::Local || parent == nullptr)
			return localUpVector;
		else if (space == Space::World)
			return parent->GetRotationModel(space) * localUpVector;
		else
		{
			assert(false && "Error. Undefined enum value.");
			return {};
		}
	}

	Math::Vector<3, float> SceneObject::GetRightVector(Space space) const
	{
		const auto& s = localRotation.GetS();
		const auto& x = localRotation.GetX();
		const auto& y = localRotation.GetY();
		const auto& z = localRotation.GetZ();

		Math::Vector<3, float> localRightVector
		{
			1 - 2 * Math::Sqrd(y) - 2 * Math::Sqrd(z),
			2 * x * y + 2 * z * s,
			2 * x * z - 2 * y * s
		};

		if (space == Space::Local || parent == nullptr)
			return localRightVector;
		else if (space == Space::World)
			return parent->GetRotationModel(space) * localRightVector;
		else
		{
			assert(false && "Error. Undefined enum value.");
			return {};
		}
	}
}