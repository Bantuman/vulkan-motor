#pragma once

#include <core/common.hpp>
#include <file/file.hpp>

class OSInputFile final : public InputFile {
	public:
		OSInputFile();

		[[nodiscard]] bool open(const char* path);
		void close();
		bool is_open() const;

		bool read(void* buffer, size_t size) override;
		const void* get_buffer() const override;
		size_t get_size() const override;

		~OSInputFile();
	private:
		NULL_COPY_AND_ASSIGN(OSInputFile);

		void* m_handle;
};

class OSOutputFile final : public OutputFile {
	public:
		OSOutputFile();

		[[nodiscard]] bool open(const char* path);
		void close();
		bool is_open() const;

		bool write(const void* buffer, size_t size) override;

		void flush();

		template <typename T>
		bool write(const T& value) {
			return write(value, sizeof(T));
		}

		~OSOutputFile();
	private:
		NULL_COPY_AND_ASSIGN(OSOutputFile);

		void* m_handle;
};

