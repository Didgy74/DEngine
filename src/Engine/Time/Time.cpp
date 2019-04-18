#include "DEngine/Time/Time.hpp"

#include <chrono>
#include <memory>
#include <array>

namespace Engine
{
	namespace Time
	{
		struct TimeData
		{
			std::chrono::high_resolution_clock::time_point appStart;
		};
		std::unique_ptr<TimeData> data;

		void Core::Initialize()
		{
			data = std::make_unique<TimeData>();

			auto now = std::chrono::high_resolution_clock::now();
			data->appStart = now;
		}

		void Core::Terminate()
		{
			data = nullptr;
		}

		void Core::TickEnd(SceneData& scene)
		{
			auto now = std::chrono::high_resolution_clock::now();

			scene.tickCount++;

			// Calculate deltaTime
			std::chrono::high_resolution_clock::duration timeBetweenFrames = now - scene.previousFrameEndTime;
			scene.deltaTime = std::chrono::duration<float>{ timeBetweenFrames }.count();

			std::chrono::high_resolution_clock::duration timeSinceSceneStart = now - scene.startTime;
			scene.timeSinceSceneStartSynced = std::chrono::duration<float>{ timeSinceSceneStart }.count();

			scene.previousFrameEndTime = now;
		}

		SceneData::SceneData()
		{
			auto now = std::chrono::high_resolution_clock::now();

			startTime = now;

			previousFrameEndTime = now;

			deltaTime = 1.f / 60.f;

			constexpr std::chrono::high_resolution_clock::duration defaultFixedTickInterval = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)) / 60;
			fixedTickInterval[0] = defaultFixedTickInterval;
			fixedTickInterval[1] = defaultFixedTickInterval;
		}

		float SceneData::GetTimeSinceSceneStart() const
		{
			return timeSinceSceneStartSynced;
		}

		float SceneData::GetDeltaTime() const { return deltaTime; }

		float SceneData::GetFixedDeltaTime() const
		{
			const auto& activeInterval = fixedTickInterval[fixedTickIntervalBufferIndex];
			return std::chrono::duration<float>(activeInterval).count();
		}

		uint16_t SceneData::GetFPS() const { return static_cast<uint16_t>(1.f / deltaTime); }

		size_t SceneData::GetTickCount() const { return tickCount; }
	}
}


