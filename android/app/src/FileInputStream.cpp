#include <DEngine/Platform/PlatformImpl.hpp>
#include <DEngine/Platform/PlatformAssert.hpp>

#include "BackendData.hpp"

#include <cstring>

using namespace DEngine;
using namespace DEngine::Platform;

namespace DEngine::Platform::impl::Backend
{
	struct FileInputStreamData
	{
		AAsset* file = nullptr;
		// There is no AAsset_tell() so we need to remember the position manually.
		size_t pos = 0;
	};
}

namespace Backend = DEngine::Platform::impl::Backend;

Platform::FileInputStream::FileInputStream()
{
	static_assert(sizeof(Backend::FileInputStreamData) <= sizeof(FileInputStream::m_buffer));

	Backend::FileInputStreamData implData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
}

Platform::FileInputStream::FileInputStream(char const* path)
{
    Backend::FileInputStreamData implData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	Open(path);
}

Platform::FileInputStream::FileInputStream(Std::Span<char const> path) {
	Backend::FileInputStreamData implData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	std::string temp;
	temp.append(path.Data(), path.Data() + path.Size());
	Open(temp.c_str());
}

Platform::FileInputStream::FileInputStream(FileInputStream&& other) noexcept
{
	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(Backend::FileInputStreamData));
    Backend::FileInputStreamData implData{};
	std::memcpy(&other.m_buffer[0], &implData, sizeof(implData));
}

Platform::FileInputStream::~FileInputStream()
{
	Close();
}

Platform::FileInputStream& DEngine::Platform::FileInputStream::operator=(FileInputStream&& other) noexcept
{
	if (this == &other)
		return *this;
	Close();

	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(Backend::FileInputStreamData));
    Backend::FileInputStreamData implData{};
	std::memcpy(&other.m_buffer[0], &implData, sizeof(implData));

	return *this;
}


bool Platform::FileInputStream::Seek(i64 offset, SeekOrigin origin)
{
    Backend::FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));

	if (implData.file == nullptr)
		return false;

	int posixOrigin = 0;
	switch (origin)
	{
		case SeekOrigin::Current:
			posixOrigin = SEEK_CUR;
			break;
		case SeekOrigin::Start:
			posixOrigin = SEEK_SET;
			break;
		case SeekOrigin::End:
			posixOrigin = SEEK_END;
			break;
	}
	off64_t result = AAsset_seek64(implData.file, static_cast<off64_t>(offset), posixOrigin);
	if (result == -1)
		return false;

	implData.pos = static_cast<size_t>(result);
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	return true;
}

bool Platform::FileInputStream::Read(char* output, u64 size)
{
    Backend::FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file == nullptr)
		return false;

	int result = AAsset_read(implData.file, output, (size_t)size);
	if (result < 0)
		return false;

	implData.pos += size;
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	return true;
}

Std::Opt<u64> Platform::FileInputStream::Tell() const
{
    Backend::FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file == nullptr)
		return {};

	return Std::Opt{ (u64)implData.pos };
}

bool Platform::FileInputStream::IsOpen() const
{
    Backend::FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	return implData.file != nullptr;
}

bool Platform::FileInputStream::Open(char const* path)
{
	Close();

    Backend::FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	auto const& backendData = *Backend::pBackendDataInit;
	implData.file = AAssetManager_open(backendData.assetManager, path, AASSET_MODE_RANDOM);
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
	return implData.file != nullptr;
}

void Platform::FileInputStream::Close()
{
    Backend::FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file != nullptr)
		AAsset_close(implData.file);

	implData = Backend::FileInputStreamData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
}

