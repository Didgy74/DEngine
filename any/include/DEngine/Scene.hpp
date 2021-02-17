#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Pair.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Utility.hpp>

// Temp
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Physics.hpp>
#include <box2d/box2d.h>

namespace DEngine
{
	struct Box2D_Component
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
		// And lots of the objects in it require a pointer to it.
		Std::Box<b2World> physicsWorld{ new b2World{{ 0.f, -10.f }} };

		Std::Span<Entity const> GetEntities() const { return { entities.data(), entities.size() }; }

		template<typename T>
		Std::Span<Std::Pair<Entity, T>> GetAllComponents() = delete;

	private:
		std::vector<Entity>& Internal_GetEntities() { return entities; }

		template<typename T>
		std::vector<Std::Pair<Entity, T>>& Internal_GetAllComponents() = delete;
		template<>
		std::vector<Std::Pair<Entity, Transform>>& Internal_GetAllComponents<Transform>() { return transforms; }
		template<>
		std::vector<Std::Pair<Entity, Gfx::TextureID>>& Internal_GetAllComponents<Gfx::TextureID>() { return textureIDs; }
		template<>
		std::vector<Std::Pair<Entity, Move>>& Internal_GetAllComponents<Move>() { return moves; }
		template<>
		std::vector<Std::Pair<Entity, Box2D_Component>>& Internal_GetAllComponents<Box2D_Component>() { return b2Bodies; }
	public:


		Entity NewEntity()
		{
			Entity returnVal = (Entity)entityIdIncrementor;
			entityIdIncrementor += 1;
			entities.push_back(returnVal);
			return returnVal;
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

			auto& componentVector = Internal_GetAllComponents<T>();
			// Crash if we already got this component
			DENGINE_DETAIL_ASSERT(GetComponent<T>(entity) == nullptr);

			componentVector.push_back({ entity, component });
		}
		template<typename T>
		void DeleteComponent(Entity entity)
		{
			DENGINE_DETAIL_ASSERT(ValidateEntity(entity));
			auto& componentVector = Internal_GetAllComponents<T>();
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
			auto& componentVector = Internal_GetAllComponents<T>();
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
			auto& componentVector = Internal_GetAllComponents<T>();
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
		std::vector<Std::Pair<Entity, Box2D_Component>> b2Bodies;
		std::vector<Entity> entities;
	};

	template<>
	inline Std::Span<Std::Pair<Entity, Transform>> Scene::GetAllComponents<Transform>() { return { transforms.data(), transforms.size() }; }
	template<>
	inline Std::Span<Std::Pair<Entity, Gfx::TextureID>> Scene::GetAllComponents<Gfx::TextureID>() { return { textureIDs.data(), textureIDs.size() }; }
	template<>
	inline Std::Span<Std::Pair<Entity, Move>> Scene::GetAllComponents<Move>() { return { moves.data(), moves.size() }; }
	template<>
	inline Std::Span<Std::Pair<Entity, Box2D_Component>> Scene::GetAllComponents<Box2D_Component>() { return { b2Bodies.data(), b2Bodies.size() }; }

}

