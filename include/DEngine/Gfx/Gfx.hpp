#pragma once

#include "DEngine/Int.hpp"
#include "DEngine/Containers/Span.hpp"
#include "DEngine/Containers/Optional.hpp"
#include "DEngine/Containers/Array.hpp"

namespace DEngine::Gfx
{
	constexpr uSize apiDataBufferSize = 4096;

	class ILog;
	struct InitInfo;

	class Data
	{
	private:
		const ILog* iLog = nullptr;

		Cont::Array<u8, apiDataBufferSize> apiDataBuffer{};

		inline Data() = default;

		friend Cont::Opt<Data> Initialize(const InitInfo&);
		friend void Draw(Data&, float);
	};

	struct InitInfo
	{
		bool (*createVkSurfacePFN)(u64 vkInstance, void* userData, u64* vkSurface) = nullptr;
		void* createVkSurfaceUserData = nullptr;

		const ILog* iLog = nullptr;
		Cont::Span<const char*> requiredVkInstanceExtensions{};
	};

	class ILog
	{
	public:
		virtual void log(const char* msg) const = 0;
		virtual ~ILog() {};
	};

	Cont::Opt<Data> Initialize(const InitInfo& initInfo);

	void Draw(Data& data, float scale);
}