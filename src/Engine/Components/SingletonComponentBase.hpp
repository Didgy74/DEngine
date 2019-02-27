#pragma once

namespace Engine
{
    class SceneObject;
}

#include "../SceneObject.hpp"

namespace Engine
{
    class SingletonComponentBase
    {
    public:
        static constexpr bool isSingleton = true;

        SingletonComponentBase(SceneObject& owningObject, size_t indexInScene) noexcept;
        SingletonComponentBase(const SingletonComponentBase& right) = delete;
        SingletonComponentBase(SingletonComponentBase&& right) noexcept = delete;

        virtual ~SingletonComponentBase() = 0;

        SingletonComponentBase& operator=(const SingletonComponentBase& right) = delete;
        SingletonComponentBase& operator=(SingletonComponentBase&& right) = delete;

        [[nodiscard]] SceneObject& GetSceneObject();
        [[nodiscard]] const SceneObject& GetSceneObject() const;
        [[nodiscard]] size_t GetIndexInScene() const;

    private:
        std::reference_wrapper<SceneObject> sceneObjectRef;
        size_t indexInScene;
    };
}

