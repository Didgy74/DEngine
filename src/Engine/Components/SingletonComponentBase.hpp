#pragma once

#include <cstdint>
#include <functional>

namespace Engine
{
    class SceneObject;
}

namespace Engine
{
    class SingletonComponentBase
    {
    public:
        static constexpr bool isSingleton = true;

        explicit SingletonComponentBase(SceneObject& owningObject);

        virtual ~SingletonComponentBase() = 0;

        [[nodiscard]] SceneObject& GetSceneObject();
        [[nodiscard]] const SceneObject& GetSceneObject() const;
        [[nodiscard]] size_t GetIndexInScene() const;

        [[nodiscard]] size_t GetID() const;
    private:
        std::reference_wrapper<SceneObject> sceneObjectRef;
    };
}

