#pragma once

#include "DynamicDispatch.hpp"

#include <DEngine/Std/Containers/AllocRef.hpp>

// For file IO
#include <DEngine/Platform/Platform.hpp>

import DEngine.Std.Vec;

namespace DEngine::Gfx::Vk::ShaderHelpers {
	[[nodiscard]] inline BoxVkShaderModule LoadShaderModuleFromFile(
		DeviceDispatch const& device,
		Std::Span<char const> filePath,
		Std::AllocRef const& transientAlloc)
	{
		Platform::FileInputStream file{ filePath };
		if (!file.IsOpen()) {
			throw std::runtime_error{"Could not open file" };
		}
		file.Seek(0, Platform::FileInputStream::SeekOrigin::End);
		u64 vertFileLength = file.Tell().Value();
		file.Seek(0, Platform::FileInputStream::SeekOrigin::Start);
		auto vertCode = Std::NewVec<char>(transientAlloc);
		vertCode.Resize((uSize)vertFileLength);
		file.Read(vertCode.Data(), vertFileLength);

		vk::ShaderModuleCreateInfo vertModCreateInfo = {};
		vertModCreateInfo.codeSize = vertCode.Size();
		vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.Data());

		return device.CreateBox(vertModCreateInfo);
	}
}
