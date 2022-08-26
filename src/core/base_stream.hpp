#pragma once

#include <cstddef>

class OutputStream {
	public:
		virtual bool write(const void* buffer, size_t size) = 0;

		virtual ~OutputStream() = default;
};

class InputStream {
	public:
		virtual bool read(void* buffer, size_t size) = 0;
		virtual const void* get_buffer() const = 0;
		virtual size_t get_size() const = 0;

		virtual ~InputStream() = default;
};

