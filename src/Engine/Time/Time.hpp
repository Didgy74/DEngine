#pragma once

namespace Engine
{
    class Scene;
}

#include <cstdint>
#include <cstddef>
#include <array>
#include <chrono>

namespace Engine
{
	namespace Time
	{
		class Core;

		class SceneData
		{
		public:
			SceneData();

			float GetTimeSinceAppStart() const;
			float GetTimeSinceSceneStart() const;
			size_t GetTickCount() const;
			float GetFixedDeltaTime() const;
			void SetFixedTickInterval(float intervalLength);
			float GetDeltaTime() const;
			uint16_t GetFPS() const;

		private:
			size_t tickCount;
			size_t fixedTickCount;
            std::chrono::high_resolution_clock::time_point startTime;
            std::chrono::high_resolution_clock::time_point previousFrameEndTime;
            float deltaTime;

            std::array<std::chrono::high_resolution_clock::duration, 2> fixedTickInterval;
            uint8_t fixedTickIntervalBufferIndex;
            bool fixedTickIntervalChanged;

            friend Time::Core;
		};

		class Core
		{
		public:
			static void Initialize();
			static void Terminate();
			static void TickEnd();
			static void TickEnd(Time::SceneData& scene);
		};
	}
}
