#include "Time.hpp"

#include "Scene.hpp"

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
	}
}

void Engine::Time::Core::Initialize()
{
	data = std::make_unique<TimeData>();

    auto now = std::chrono::high_resolution_clock::now();
	data->appStart = now;
}

void Engine::Time::Core::Terminate()
{
	data = nullptr;
}

void Engine::Time::Core::TickEnd(SceneData& scene)
{
	auto now = std::chrono::high_resolution_clock::now();

	scene.tickCount++;

	if (scene.fixedTickIntervalChanged)
	{
		// Can't use the literal 1 in the expression, since it throws a narrowing warning.
		// Static-cast is just way too verbose in this case, but the C++-purist in me didn't allow a C-style cast.
		// That's why I have this ridiculous constant here.
		constexpr uint8_t one = 1;
		scene.fixedTickIntervalBufferIndex = (scene.fixedTickIntervalBufferIndex + one) % static_cast<uint8_t>(scene.fixedTickInterval.size() - 1);
		scene.fixedTickIntervalChanged = false;
	}

	// Calculate deltaTime
	std::chrono::high_resolution_clock::duration timeBetweenFrames = now - scene.previousFrameEndTime;
	scene.deltaTime = std::chrono::duration<float>{timeBetweenFrames}.count();

	scene.previousFrameEndTime = now;
}

Engine::Time::SceneData::SceneData() :
    tickCount(0),
    fixedTickInterval(),
    fixedTickIntervalChanged(false),
    fixedTickIntervalBufferIndex(0)
{
    auto now = std::chrono::high_resolution_clock::now();

    startTime = now;

    previousFrameEndTime = now;

	deltaTime = 1.f / 60.f;

	constexpr std::chrono::high_resolution_clock::duration defaultFixedTickInterval = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)) / 60;
	fixedTickInterval[0] = defaultFixedTickInterval;
}

float Engine::Time::SceneData::GetDeltaTime() const { return deltaTime; }

float Engine::Time::SceneData::GetFixedDeltaTime() const
{
	const auto& activeInterval = fixedTickInterval[fixedTickIntervalBufferIndex];
	return std::chrono::duration<float>(activeInterval).count();
}

uint16_t Engine::Time::SceneData::GetFPS() const { return static_cast<uint16_t>(1.f / deltaTime); }
