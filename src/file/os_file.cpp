#include "os_file.hpp"

#include <cassert>
#include <cstdio>

// OSInputFile

OSInputFile::OSInputFile()
		: m_handle(nullptr) {}

bool OSInputFile::open(const char* path) {
	m_handle = fopen(path, "rb");

	return is_open();
}

void OSInputFile::close() {
	if (is_open()) {
		fclose(static_cast<FILE*>(m_handle));
		m_handle = nullptr;
	}
}

bool OSInputFile::is_open() const {
	return m_handle != nullptr;
}

bool OSInputFile::read(void* buffer, size_t size) {
	assert(is_open());

	const size_t numRead = fread(buffer, size, 1, static_cast<FILE*>(m_handle));
	return numRead == 1;
}

const void* OSInputFile::get_buffer() const {
	return nullptr;
}

size_t OSInputFile::get_size() const {
	assert(is_open());

	long pos = ftell(static_cast<FILE*>(m_handle));
	fseek(static_cast<FILE*>(m_handle), 0, SEEK_END);

	size_t size = static_cast<size_t>(ftell(static_cast<FILE*>(m_handle)));
	fseek(static_cast<FILE*>(m_handle), pos, SEEK_SET);

	return size;
}

OSInputFile::~OSInputFile() {
	close();
}

// OSOutputFile

OSOutputFile::OSOutputFile()
		: m_handle(nullptr) {}

bool OSOutputFile::open(const char* path) {
	m_handle = fopen(path, "wb");

	return is_open();
}

void OSOutputFile::close() {
	if (is_open()) {
		fclose(static_cast<FILE*>(m_handle));
		m_handle = nullptr;
	}
}

bool OSOutputFile::is_open() const {
	return m_handle != nullptr;
}

bool OSOutputFile::write(const void* buffer, size_t size) {
	assert(is_open());

	const size_t numWritten = fwrite(buffer, size, 1, static_cast<FILE*>(m_handle));
	return numWritten == 1;
}

void OSOutputFile::flush() {
	assert(is_open());
	fflush(static_cast<FILE*>(m_handle));
}

OSOutputFile::~OSOutputFile() {
	close();
}

