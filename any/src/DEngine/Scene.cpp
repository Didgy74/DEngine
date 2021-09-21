#include <DEngine/Scene.hpp>

using namespace DEngine;

void Scene::Copy(Scene& output) const
{
	output.entities = entities;
	output.entityIdIncrementor = entityIdIncrementor;
	output.moves = moves;
	output.rigidBodies = rigidBodies;
	output.textureIDs = textureIDs;
	output.transforms = transforms;

	// Don't need to copy physics world, it's not initialized anyways.
}

Entity Scene::NewEntity() noexcept
{
	Entity returnVal = (Entity)entityIdIncrementor;
	entityIdIncrementor += 1;
	entities.push_back(returnVal);
	return returnVal;
}

void Scene::DeleteEntity(Entity ent) noexcept
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
bool Scene::ValidateEntity(Entity entity) const noexcept
{
	for (auto const& item : entities)
	{
		if (item == entity)
			return true;
	}
	return false;
}

void Scene::Begin()
{
	DENGINE_DETAIL_ASSERT(!physicsWorld);

	auto physWorld = new b2World({ 0.f, -10.f });
	physicsWorld = Std::Box{ physWorld };

	for (auto& [entity, rb] : GetAllComponents<Physics::Rigidbody2D>())
	{
		auto transformPtr = GetComponent<Transform>(entity);
		if (!transformPtr)
			continue;
		auto const& transform = *transformPtr;

		b2BodyDef bodyDef = {};
		bodyDef.angle = transform.rotation;
		bodyDef.awake = true;
		bodyDef.enabled = true;
		bodyDef.gravityScale = 1.f;
		bodyDef.position = { transform.position.x, transform.position.y };
		switch (rb.type)
		{
		case Physics::Rigidbody2D::Type::Dynamic:
			bodyDef.type = b2BodyType::b2_dynamicBody;
			break;
		case Physics::Rigidbody2D::Type::Static:
			bodyDef.type = b2BodyType::b2_staticBody;
			break;
		default:
			DENGINE_IMPL_UNREACHABLE();
			break;
		}
		b2Body* newBody = physWorld->CreateBody(&bodyDef);

		rb.b2BodyPtr = newBody;

		b2FixtureDef fixtureDef{};
		fixtureDef.density = 1.f;
		fixtureDef.friction = 1.f;
		fixtureDef.restitution = 0.f;

		b2PolygonShape shape = {};
		shape.SetAsBox(transform.scale.x / 2.f, transform.scale.y / 2.f);
		fixtureDef.shape = &shape;

		newBody->CreateFixture(&fixtureDef);
	}
}