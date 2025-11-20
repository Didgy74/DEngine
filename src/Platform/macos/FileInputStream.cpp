#include <DEngine/Platform/PlatformImpl.hpp>

#include <cstdio>

using namespace DEngine;

Platform::FileInputStream::FileInputStream() {
	static_assert(sizeof(std::FILE*) <= sizeof(FileInputStream::m_buffer));
}

Platform::FileInputStream::FileInputStream(char const* path) {
	Open(path);
}

Platform::FileInputStream::FileInputStream(Std::Span<char const> path) {
	std::string temp;
	temp.append(path.Data(), path.Data() + path.Size());
	Open(temp.c_str());
}

Platform::FileInputStream::FileInputStream(FileInputStream&& other) noexcept {
	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(std::FILE*));
	std::memset(&other.m_buffer[0], 0, sizeof(std::FILE*));
}

Platform::FileInputStream::~FileInputStream() {
	Close();
}

Platform::FileInputStream& Platform::FileInputStream::operator=(FileInputStream&& other) noexcept {
	if (this == &other)
		return *this;

	Close();

	std::memcpy(&this->m_buffer[0], &other.m_buffer[0], sizeof(std::FILE*));
	std::memset(&other.m_buffer[0], 0, sizeof(std::FILE*));

	return *this;
}

bool Platform::FileInputStream::Seek(i64 offset, SeekOrigin origin) {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file == nullptr)
		return false;

	int posixOrigin = 0;
	switch (origin) {
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
	int result = fseek(file, (long) offset, posixOrigin);
	return result == 0;
}

bool Platform::FileInputStream::Read(char* output, u64 size) {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file == nullptr)
		return false;

	size_t result = std::fread(output, 1, (size_t) size, file);
	return result == (size_t) size;
}

Std::Opt<u64> Platform::FileInputStream::Tell() const {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file == nullptr)
		return {};

	long result = ftell(file);
	if (result == long(-1))
		// Handle error
		return {};
	else
		return Std::Opt{static_cast<u64>(result)};
}

bool Platform::FileInputStream::IsOpen() const {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	return file != nullptr;
}

bool Platform::FileInputStream::Open(char const* path) {
	Close();
	std::FILE* file = std::fopen(path, "rb");
	std::memcpy(&m_buffer[0], &file, sizeof(std::FILE*));
	return file != nullptr;
}

void Platform::FileInputStream::Close() {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file != nullptr)
		std::fclose(file);

	std::memset(&m_buffer[0], 0, sizeof(std::FILE*));
}