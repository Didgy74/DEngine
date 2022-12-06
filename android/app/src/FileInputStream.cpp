#include <DEngine/impl/Application.hpp>
#include <DEngine/impl/AppAssert.hpp>

#include "BackendData.hpp"

#include <cstring>

using namespace DEngine;
using namespace DEngine::Application;

namespace DEngine::Application::impl
{
	struct Backend_FileInputStreamData
	{
		AAsset* file = nullptr;
		// There is no AAsset_tell() so we need to remember the position manually.
		size_t pos = 0;
	};
}

Application::FileInputStream::FileInputStream()
{
	static_assert(sizeof(impl::Backend_FileInputStreamData) <= sizeof(FileInputStream::m_buffer));

	impl::Backend_FileInputStreamData implData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
}

Application::FileInputStream::FileInputStream(char const* path)
{
	impl::Backend_FileInputStreamData implData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	Open(path);
}

Application::FileInputStream::FileInputStream(FileInputStream&& other) noexcept
{
	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(impl::Backend_FileInputStreamData));
	impl::Backend_FileInputStreamData implData{};
	std::memcpy(&other.m_buffer[0], &implData, sizeof(implData));
}

Application::FileInputStream::~FileInputStream()
{
	Close();
}

Application::FileInputStream& DEngine::Application::FileInputStream::operator=(FileInputStream&& other) noexcept
{
	if (this == &other)
		return *this;
	Close();

	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(impl::Backend_FileInputStreamData));
	impl::Backend_FileInputStreamData implData{};
	std::memcpy(&other.m_buffer[0], &implData, sizeof(implData));

	return *this;
}


bool Application::FileInputStream::Seek(i64 offset, SeekOrigin origin)
{
	impl::Backend_FileInputStreamData implData{};
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

bool Application::FileInputStream::Read(char* output, u64 size)
{
	impl::Backend_FileInputStreamData implData{};
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

Std::Opt<u64> Application::FileInputStream::Tell() const
{
	impl::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file == nullptr)
		return {};

	return Std::Opt{ (u64)implData.pos };
}

bool Application::FileInputStream::IsOpen() const
{
	impl::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	return implData.file != nullptr;
}

bool Application::FileInputStream::Open(char const* path)
{
	Close();

	impl::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	auto const& backendData = *impl::pBackendDataInit;
	implData.file = AAssetManager_open(backendData.assetManager, path, AASSET_MODE_RANDOM);
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
	return implData.file != nullptr;
}

void Application::FileInputStream::Close()
{
	impl::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file != nullptr)
		AAsset_close(implData.file);

	implData = impl::Backend_FileInputStreamData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
}

