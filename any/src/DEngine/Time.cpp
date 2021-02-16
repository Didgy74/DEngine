#include "DEngine/Time.hpp"

#include <chrono>

namespace DEngine::Time
{
	static u64 tickCount = 0;
	static std::chrono::high_resolution_clock::time_point tickStartTimePoint{};
	static f32 deltaTime = 1 / 60.f;

	f32 Delta()
	{
		return deltaTime;
	}

	u64 TickCount()
	{
		return tickCount;
	}

	void Initialize()
	{
		auto now = std::chrono::high_resolution_clock::now();
		tickStartTimePoint = now;
	}

	void TickStart()
	{
		auto now = std::chrono::high_resolution_clock::now();

		tickCount += 1;


		// Calculate deltaTime
		std::chrono::high_resolution_clock::duration timeBetweenFrames = now - tickStartTimePoint;
		deltaTime = std::chrono::duration<f32>{ timeBetweenFrames }.count();

		tickStartTimePoint = now;
	}
}

