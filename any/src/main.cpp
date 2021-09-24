#include <DEngine/impl/Application.hpp>
#include "DEngine/Editor/Editor.hpp"
#include "DEngine/Gfx/Gfx.hpp"

#include <DEngine/Scene.hpp>
#include <DEngine/Time.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Utility.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>

#include <iostream>
#include <vector>
#include <string>

#include <Tracy.hpp>

void DEngine::Move::Update(Entity entity, Scene& scene, f32 deltaTime) const
{
	auto const rbPtr = scene.GetComponent<Physics::Rigidbody2D>(entity);
	b2Body* physBodyPtr = nullptr;
	if (rbPtr != nullptr)
	{
		DENGINE_IMPL_ASSERT(rbPtr->b2BodyPtr);
		physBodyPtr = (b2Body*)rbPtr->b2BodyPtr;
	}
	if (!physBodyPtr)
		return;
	auto& physBody = *physBodyPtr;

	auto const gamepadOpt = App::GetGamepad();
	if (!gamepadOpt.HasValue())
		return;

	auto const& gamepadState = gamepadOpt.Value();
	if (gamepadState.GetKeyEvent(App::GamepadKey::A) == App::KeyEventType::Pressed)
	{
		physBody.ApplyLinearImpulseToCenter({ 0.f, 5.f }, true);
	}

	auto leftStickX = gamepadState.GetGamepadAxisValue(App::GamepadAxis::LeftX);
	if (Math::Abs(leftStickX) > gamepadState.stickDeadzone)
	{
		leftStickX *= 2.f;

		auto const currVelocity = physBody.GetLinearVelocity();
		physBody.SetLinearVelocity({ leftStickX, currVelocity.y });
		physBody.SetAwake(true);
	}
}

namespace DEngine::impl
{
	class GfxLogger : public Gfx::LogInterface
	{
	public:
		virtual void Log(Gfx::LogInterface::Level level, const char* msg) override
		{
			App::Log(msg);
		}

		virtual ~GfxLogger() override
		{

		}
	};

	class GfxTexAssetInterfacer : public Gfx::TextureAssetInterface
	{
		virtual char const* get(Gfx::TextureID id) const override
		{
			switch ((int)id)
			{
				case 0:
					return "data/Ground.png";
				case 1:
					return "data/Crate.png";
				case 2:
					return "data/02.png";
				default:
					return "data/01.ktx";
			}
		}
	};

	struct GfxWsiConnection : public Gfx::WsiInterface
	{
		App::WindowID appWindowID{};

		// Return type is VkResult
		//
		// Argument #1: VkInstance - The Vulkan instance handle
		// Argument #2: VkAllocationCallbacks const* - Allocation callbacks for surface creation.
		// Argument #3: VkSurfaceKHR* - The output surface handle
		virtual i32 CreateVkSurface(uSize vkInstance, void const* allocCallbacks, u64& outSurface) override
		{
			auto resultOpt = App::CreateVkSurface(appWindowID, vkInstance, nullptr);
			if (resultOpt.HasValue())
			{
				outSurface = resultOpt.Value();
				return 0; // 0 is VK_RESULT_SUCCESS
			}
			else
				return -1;
		}
	};

	Gfx::Context CreateGfxContext(
		Gfx::WsiInterface& wsiConnection,
		Gfx::TextureAssetInterface const& textureAssetConnection,
		Gfx::LogInterface& logger,
		Std::Span<char const*> requiredVkInstanceExtensions)
	{
		Gfx::InitInfo rendererInitInfo = {};
		rendererInitInfo.initialWindowConnection = &wsiConnection;
		rendererInitInfo.texAssetInterface = &textureAssetConnection;
		rendererInitInfo.optional_logger = &logger;
		rendererInitInfo.requiredVkInstanceExtensions = requiredVkInstanceExtensions;
		rendererInitInfo.gizmoArrowMesh = Editor::BuildGizmoTranslateArrowMesh2D();
		rendererInitInfo.gizmoCircleLineMesh = Editor::BuildGizmoTorusMesh2D();
		rendererInitInfo.gizmoArrowScaleMesh2d = Editor::BuildGizmoScaleArrowMesh2D();
		Std::Opt<Gfx::Context> rendererDataOpt = Gfx::Initialize(rendererInitInfo);
		if (!rendererDataOpt.HasValue())
		{
			std::cout << "Could not initialize renderer." << std::endl;
			std::abort();
		}
		return static_cast<Gfx::Context&&>(rendererDataOpt.Value());
	}

	void CopyTransformToPhysicsWorld(
		Scene& scene);

	void RunPhysicsStep(
		Scene& scene);
	
	void SubmitRendering(
		Gfx::Context& gfxData,
		Editor::Context& editorData,
		Scene& scene);
}

/*
namespace DEngine::impl
{
	[[nodiscard]] constexpr unsigned long long FNV_1a_Hash(char const* str, unsigned long long length) noexcept
	{
		constexpr unsigned long long FNV_Basis = 14695981039346656037ULL;
		constexpr unsigned long long FNV_Prime = 1099511628211ULL;
		unsigned long long hasher = FNV_Basis;
		for (unsigned long long i = 0; i < length; i += 1)
		{
			hasher = hasher ^ str[i];
			hasher = hasher * FNV_Prime;
		}
		return hasher;
	}

	// Should NOT include null-terminator.
	struct CompileTimeString
	{
		char const* ptr;
		unsigned long long length;
	};

	// Lengths should NOT include null-terminator.
	// Returns index if found. Returns -1 if not found.
	[[nodiscard]] constexpr signed long long FindSubstring(
		char const* srcPtr,
		unsigned long long srcLength,
		char const* substringPtr,
		unsigned long long substringLength) noexcept
	{
		constexpr auto failValue = (signed long long)(-1);

		unsigned long long lengthToSearch = substringLength > srcLength ? 0 : srcLength - substringLength + 1;
		// Check every substring in src
		for (unsigned long long i = 0; i < lengthToSearch; i += 1)
		{
			char const* srcSubstring = srcPtr + i;
			bool found = true;
			// Compare substring to toFind.
			for (unsigned long long j = 0; j < substringLength; j += 1)
			{
				char const a = srcSubstring[j];
				char const b = substringPtr[j];
				if (a != b)
				{
					found = false;
					break;
				}
			}
			if (found)
				return i;
		}

		return failValue;
	}

	template<typename T>
	[[nodiscard]] constexpr auto GetPrettyFunctionString() noexcept
	{
#if defined(__clang__)
#	define DENGINE_FN_SIGNATURE __FUNCSIG__
#	define DENGINE_FN_PREFIX "auto __cdecl DEngine::impl::GetPrettyFunctionString(void) [T = "
#	define DENGINE_FN_POSTFIX "]"
#elif defined(_MSC_VER)
#	define DENGINE_FN_SIGNATURE __FUNCSIG__
#	define DENGINE_FN_PREFIX "auto __cdecl DEngine::impl::GetPrettyFunctionString<"
#	define DENGINE_FN_POSTFIX ">(void) noexcept"
#else
#	error "Need to implement function name thing"
#endif
		constexpr const char* name = DENGINE_FN_SIGNATURE;
		constexpr auto nameLength = sizeof(DENGINE_FN_SIGNATURE) - 1;

		constexpr const char* preFix = DENGINE_FN_PREFIX;
		constexpr auto preFixLength = sizeof(DENGINE_FN_PREFIX) - 1;

		constexpr const char* postFix = DENGINE_FN_POSTFIX;
		constexpr auto postFixLength = sizeof(DENGINE_FN_POSTFIX) - 1;
#undef DENGINE_FN_SIGNATURE
#undef DENGINE_FN_PREFIX
#undef DENGINE_FN_POSTFIX

		char const* returnPtr = name;
		unsigned long long returnLength = nameLength;

		// Filter away the front
		static_assert(nameLength > preFixLength, "Needs to be able to fit the postfix string in source string.");
		// Assert that the source has this substring at the very front
		constexpr auto preFixIndex = FindSubstring(name, preFixLength, preFix, preFixLength);
		static_assert(preFixIndex == 0, "Could not find the preFix string in the source string.");

		returnPtr += preFixLength;
		returnLength -= preFixLength;

		// Filter away the rear
		static_assert(nameLength > postFixLength, "Needs to be able to fit the postfix string in source string.");
		// Assert that the source has this substring at the very end
		constexpr auto postFixIndex = FindSubstring(name + nameLength - postFixLength, postFixLength, postFix, postFixLength);
		static_assert(postFixIndex == 0, "Could not find the postFix string in the source string.");

		returnLength -= postFixLength;

		// Filter away class keyword
		constexpr char classLabel[] = "class ";
		char const* classLabelPtr = classLabel;
		constexpr unsigned long long classLabelLength = sizeof(classLabel) - 1;
		if (FindSubstring(returnPtr, returnLength, classLabelPtr, classLabelLength) == 0)
		{
			returnPtr += classLabelLength + 1;
			returnLength -= classLabelLength + 1;
		}

		// Filter away struct keyword
		constexpr char structLabel[] = "struct ";
		char const* structLabelPtr = structLabel;
		constexpr unsigned long long structLabelLength = sizeof(structLabel) - 1;
		if (FindSubstring(returnPtr, returnLength, structLabelPtr, structLabelLength) == 0)
		{
			returnPtr += classLabelLength + 1;
			returnLength -= classLabelLength + 1;
		}

		return CompileTimeString{ returnPtr, returnLength };
	}

	[[nodiscard]] constexpr unsigned long long Hash(CompileTimeString in) noexcept
	{
		return FNV_1a_Hash(in.ptr, in.length);
	}

	template<typename T>
	constexpr unsigned long long GetTypeID() noexcept
	{
		return Hash(InitTestPipeline<T>());
	}
}
 */

int DENGINE_APP_MAIN_ENTRYPOINT(int argc, char** argv)
{
	using namespace DEngine;

	Std::NameThisThread("MainThread");

	Time::Initialize();
	App::detail::Initialize();

	App::WindowID mainWindow = App::CreateWindow(
		"Main window",
		{ 1280, 800 });

	auto gfxWsiConnection = new impl::GfxWsiConnection;
	gfxWsiConnection->appWindowID = mainWindow;

	// Initialize the renderer
	auto requiredInstanceExtensions = App::RequiredVulkanInstanceExtensions();
	impl::GfxLogger gfxLogger = {};
	impl::GfxTexAssetInterfacer gfxTexAssetInterfacer{};
	Gfx::Context gfxCtx = impl::CreateGfxContext(
		*gfxWsiConnection,
		gfxTexAssetInterfacer,
		gfxLogger,
		requiredInstanceExtensions);

	Scene myScene;

	{
		Entity ent = myScene.NewEntity();

		Transform transform = {};
		transform.position.x = 0.f;
		transform.position.y = 0.f;
		//transform.rotation = 0.707f;
		//transform.scale = { 1.f, 1.f };
		myScene.AddComponent(ent, transform);

		Gfx::TextureID textureId{ 1 };
		myScene.AddComponent(ent, textureId);

		Physics::Rigidbody2D rb = {};
		rb.type = Physics::Rigidbody2D::Type::Dynamic;
		myScene.AddComponent(ent, rb);

		Move move = {};
		myScene.AddComponent(ent, move);
	}

	{
		Entity ent = myScene.NewEntity();

		Transform transform = {};
		transform.position.x = 0.f;
		transform.position.y = -2.f;
		//transform.rotation = 0.707f;
		transform.scale.x = 5.f;
		myScene.AddComponent(ent, transform);

		Gfx::TextureID textureId{ 0 };
		myScene.AddComponent(ent, textureId);

		Physics::Rigidbody2D rb = {};
		rb.type = Physics::Rigidbody2D::Type::Static;
		myScene.AddComponent(ent, rb);

		Move move = {};
		myScene.AddComponent(ent, move);
	}

	Editor::Context editorCtx = Editor::Context::Create(
		mainWindow,
		&myScene,
		&gfxCtx);

	while (true)
	{
		Time::TickStart();
		App::detail::ProcessEvents();
		if (App::GetWindowCount() == 0)
			break;

		editorCtx.ProcessEvents();

		Scene* renderedScene = &myScene;

		if (editorCtx.IsSimulating())
		{
			Scene& scene = editorCtx.GetActiveScene();
			renderedScene = &scene;

			// Editor can move stuff around, so we need to update the physics world.
			impl::CopyTransformToPhysicsWorld(scene);

			//Physics::Update(myScene, Time::Delta());

			for (auto const& [entity, moveComponent] : scene.GetAllComponents<Move>())
				moveComponent.Update(entity, scene, Time::Delta());

			impl::RunPhysicsStep(scene);
		}

		impl::SubmitRendering(
			gfxCtx, 
			editorCtx, 
			*renderedScene);

		FrameMark
	}

	return 0;
}

void DEngine::impl::CopyTransformToPhysicsWorld(Scene& scene)
{
	// First copy our positions into every physics body
	for (auto const& physicsBodyPair : scene.GetAllComponents<Physics::Rigidbody2D>())
	{
		Entity a = physicsBodyPair.a;

		Transform const* transformPtr = scene.GetComponent<Transform>(a);
		DENGINE_IMPL_ASSERT(transformPtr != nullptr);
		Transform const& transform = *transformPtr;
		b2Body* pBody = (b2Body*)physicsBodyPair.b.b2BodyPtr;
		pBody->SetTransform({ transform.position.x, transform.position.y }, transform.rotation);
	}
}

void DEngine::impl::RunPhysicsStep(
	Scene& scene)
{
	CopyTransformToPhysicsWorld(scene);

	scene.physicsWorld->Step(Time::Delta(), 8, 8);

	// Then copy the stuff back
	for (auto const& physicsBodyPair : scene.GetAllComponents<Physics::Rigidbody2D>())
	{
		Entity a = physicsBodyPair.a;
		Transform* transformPtr = scene.GetComponent<Transform>(a);
		DENGINE_IMPL_ASSERT(transformPtr != nullptr);
		Transform& transform = *transformPtr;

		b2Body* pBody = (b2Body*)physicsBodyPair.b.b2BodyPtr;
		auto physicsBodyTransform = pBody->GetTransform();
		transform.position = { physicsBodyTransform.p.x, physicsBodyTransform.p.y };
		transform.rotation = physicsBodyTransform.q.GetAngle();
	}
}

void DEngine::impl::SubmitRendering(
	Gfx::Context& gfxData,
	Editor::Context& editorCtx,
	Scene& scene)
{
	Gfx::DrawParams params = {};

	for (auto const& item : scene.GetAllComponents<Gfx::TextureID>())
	{
		auto& entity = item.a;

		// First check if this entity has a position
		Transform* transformPtr = scene.GetComponent<Transform>(entity);
		if (transformPtr == nullptr)
			continue;
		Transform& transform = *transformPtr;

		params.textureIDs.push_back(item.b);

		Math::Mat4 transformMat = Math::LinAlg3D::Translate(transform.position) *
			Math::LinAlg3D::Rotate_Homo(Math::ElementaryAxis::Z, transform.rotation) *
			Math::LinAlg3D::Scale_Homo(transform.scale.AsVec3(1.f));
		params.transforms.push_back(transformMat);
	}

	auto editorDrawData = editorCtx.GetDrawInfo();
	params.guiVertices = editorDrawData.vertices;
	params.guiIndices = editorDrawData.indices;
	params.guiDrawCmds = editorDrawData.drawCmds;
	params.nativeWindowUpdates = editorDrawData.windowUpdates;
	params.viewportUpdates = editorDrawData.viewportUpdates;
	params.lineVertices = editorDrawData.lineVertices;
	params.lineDrawCmds = editorDrawData.lineDrawCmds;


	if (!params.nativeWindowUpdates.empty())
	{
		gfxData.Draw(params);
	}
}