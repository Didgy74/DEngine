#pragma once

#include <cstdint>
#include <functional>

namespace Engine
{
	class Scene;

	template<typename T>
	class ComponentReference
	{
	public:
		using Type = T;
		ComponentReference(Scene& scene, size_t guid);

		T* Get();
		const T* Get() const;
		size_t GetGUID() const;
	private:
		size_t guid;
		std::reference_wrapper<Scene> sceneRef;
	};

	template<typename T>
	using CompRef = ComponentReference<T>;
}

template<typename T>
Engine::ComponentReference<T>::ComponentReference(Scene &scene, size_t guid) :
	sceneRef(scene),
	guid(guid)
{
}

template<typename T>
size_t Engine::ComponentReference<T>::GetGUID() const { return guid; }