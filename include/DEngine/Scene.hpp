#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Pair.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>

// Temp
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Physics.hpp>
#include <box2d/box2d.h>

namespace DEngine
{
	struct Box2DBody
	{
		b2Body* ptr = nullptr;
	};

	enum class Entity : u64 { Invalid = u64(-1) };

	class Scene;

	struct Move
	{
		void Update(Entity entity, Scene& scene, f32 deltaTime) const;
	};

	class Transform
	{
	public:
		Math::Vec3 position{};
		f32 rotation = 0;
	};

	class Scene
	{
	public:
		Scene() = default;
		Scene(Scene const&) = delete;
		Scene(Scene&&) = default;
		Scene& operator=(Scene const&) = delete;
		Scene& operator=(Scene&&) = default;

		bool play = false;

		// We need this to be a pointer to heap because the struct is so huge.
		Std::Box<b2World> physicsWorld{ new b2World{{ 0.f, -10.f }} };

		Std::Span<Entity const> GetEntities() const { return { entities.data(), entities.size() }; }

		template<typename T>
		using ComponentVector = std::vector<Std::Pair<Entity, T>>;
		template<typename T>
		ComponentVector<T>& GetComponentVector() = delete;
		template<>
		ComponentVector<Transform>& GetComponentVector<Transform>() { return transforms; }
		template<>
		ComponentVector<Gfx::TextureID>& GetComponentVector<Gfx::TextureID>() { return textureIDs; }
		template<>
		ComponentVector<Move>& GetComponentVector<Move>() { return moves; }
		template<>
		ComponentVector<Box2DBody>& GetComponentVector<Box2DBody>() { return b2Bodies; }



		Entity NewEntity()
		{
			entities.push_back((Entity)entityIdIncrementor);
			entityIdIncrementor += 1;

			return entities[entities.size() - 1];
		}

		void DeleteEntity(Entity ent)
		{
			DENGINE_DETAIL_ASSERT(ValidateEntity(ent));
			DeleteComponent_CanFail<Transform>(ent);
			DeleteComponent_CanFail<Gfx::TextureID>(ent);
			DeleteComponent_CanFail<Move>(ent);

			for (uSize i = 0; i < entities.size(); i += 1)
			{
				if (ent == entities[i])
				{
					entities.erase(entities.begin() + i);
					break;
				}
			}
		}

		// Confirm that this entity exists.
		[[nodiscard]] bool ValidateEntity(Entity entity) const noexcept
		{
			for (auto const& item : entities)
			{
				if (item == entity)
					return true;
			}
			return false;
		}

		template<typename T>
		void AddComponent(Entity entity, T const& component)
		{
			DENGINE_DETAIL_ASSERT(ValidateEntity(entity));

			auto& componentVector = GetComponentVector<T>();
			// Crash if we already got this component
			DENGINE_DETAIL_ASSERT(GetComponent<T>(entity) == nullptr);

			componentVector.push_back({ entity, component });
		}
		template<typename T>
		void DeleteComponent(Entity entity)
		{
			DENGINE_DETAIL_ASSERT(ValidateEntity(entity));
			auto& componentVector = GetComponentVector<T>();
			auto const componentIt = Std::FindIf(
				componentVector.begin(),
				componentVector.end(),
				[entity](auto const& val) -> bool { return entity == val.a; });
			DENGINE_DETAIL_ASSERT(componentIt != componentVector.end());
			componentVector.erase(componentIt);
		}
		template<typename T>
		void DeleteComponent_CanFail(Entity entity)
		{
			auto& componentVector = GetComponentVector<T>();
			auto const componentIt = Std::FindIf(
				componentVector.begin(),
				componentVector.end(),
				[entity](auto const& val) -> bool { return entity == val.a; });
			if (componentIt != componentVector.end())
				componentVector.erase(componentIt);
		}
		template<typename T>
		[[nodiscard]] T* GetComponent(Entity entity)
		{
			DENGINE_DETAIL_ASSERT(ValidateEntity(entity));
			auto& componentVector = GetComponentVector<T>();
			auto const componentIt = Std::FindIf(
				componentVector.begin(),
				componentVector.end(),
				[entity](auto const& val) -> bool { return entity == val.a; });
			if (componentIt != componentVector.end())
				return &componentIt->b;
			else
				return nullptr;
		}

	private:
		u64 entityIdIncrementor = 0;
		std::vector<Std::Pair<Entity, Transform>> transforms;
		std::vector<Std::Pair<Entity, Gfx::TextureID>> textureIDs;
		std::vector<Std::Pair<Entity, Move>> moves;
		std::vector<Std::Pair<Entity, Box2DBody>> b2Bodies;
		std::vector<Entity> entities;
	};
}

