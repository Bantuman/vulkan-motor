#pragma once

#include <core/common.hpp>
#include <core/local.hpp>

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

enum class PathType : uint8_t {
	FILE,
	DIRECTORY,
	SPECIAL
};

struct Path {
	std::string name;
	PathType type;
};

struct FileStats {
	size_t size;
	PathType type;
	uint64_t lastModified;
};

class InputFile;
class OutputFile;
class FileSystemBackend;

class FileSystem {
	public:
		explicit FileSystem();

		void add_backend(const std::string_view& scheme,
				std::unique_ptr<FileSystemBackend> backend);

		std::vector<Path> walk(const std::string_view& path);
		std::vector<Path> list(const std::string_view& path);
		bool get_file_stats(const std::string_view& path, FileStats& stats);

		std::unique_ptr<InputFile> open_file_read(const std::string_view& path);
		std::unique_ptr<OutputFile> open_file_write(const std::string_view& path);

		std::vector<char> file_read_bytes(const std::string_view& path);

		std::string get_file_system_path(const std::string_view& path);

		FileSystemBackend* get_backend(const std::string_view& scheme);

		NULL_COPY_AND_ASSIGN(FileSystem);
	private:
		std::unordered_map<std::string, std::unique_ptr<FileSystemBackend>> m_backends;
};

class FileSystemBackend {
	public:
		std::vector<Path> walk(const std::string_view& path);

		virtual std::vector<Path> list(const std::string_view& path) = 0;
		virtual bool get_file_stats(const std::string_view& path, FileStats& stats) = 0;
		virtual std::string get_file_system_path(const std::string_view& path) = 0;

		virtual std::unique_ptr<InputFile> open_file_read(const std::string_view& path) = 0;
		virtual std::unique_ptr<OutputFile> open_file_write(const std::string_view& path) = 0;

		void set_scheme(const std::string_view& scheme);

		virtual ~FileSystemBackend() = default;
	private:
		std::string m_scheme;
};

inline Local<FileSystem> g_fileSystem;

