#pragma once

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
			size_t tickCount = 0;
			size_t fixedTickCount = 0;
			std::chrono::high_resolution_clock::time_point startTime{};
			float timeSinceSceneStartSynced = 0.f;
			std::chrono::high_resolution_clock::time_point previousFrameEndTime{};
			float deltaTime = 0.f;

			std::array<std::chrono::high_resolution_clock::duration, 2> fixedTickInterval{};
			uint8_t fixedTickIntervalBufferIndex = 0;
			bool fixedTickIntervalChanged = false;

            friend class Time::Core;
		};

		class Core
		{
		public:
			static void Initialize();
			static void InitializeData(SceneData& input);
			static void Terminate();
			static void TickEnd();
			static void TickEnd(Time::SceneData& scene);
		};
	}
}
