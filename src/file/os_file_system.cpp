#include "os_file_system.hpp"

#include <path_utils.hpp>
#include <os_file.hpp>

std::unique_ptr<InputFile> OSFileSystem::open_file_read(const std::string_view& path) {
	OSInputFile* file = new OSInputFile;
	auto fsPath = get_file_system_path(path);

	if (!file->open(fsPath.c_str())) {
		delete file;
		return nullptr;
	}

	return std::unique_ptr<InputFile>(file);
}

std::unique_ptr<OutputFile> OSFileSystem::open_file_write(const std::string_view& path) {
	OSOutputFile* file = new OSOutputFile;
	auto fsPath = get_file_system_path(path);

	if (!file->open(fsPath.c_str())) {
		delete file;
		return nullptr;
	}

	return std::unique_ptr<OutputFile>(file);
}

std::string OSFileSystem::get_file_system_path(const std::string_view& pathName) {
	return PathUtils::join(m_base, pathName);
}

#if defined(OPERATING_SYSTEM_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>

OSFileSystem::OSFileSystem(const std::string_view& base)
		: m_base(base) {}

std::vector<Path> OSFileSystem::list(const std::string_view& pathName) {
	WIN32_FIND_DATAA result;
	std::string joinedPath = PathUtils::join(m_base, pathName);

	HANDLE handle = FindFirstFileA(joinedPath.c_str(), &result);

	if (handle == INVALID_HANDLE_VALUE) {
		// TODO: LOG_ERROR
		return {};
	}

	if (result.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		joinedPath = PathUtils::join(joinedPath, "*");

		CloseHandle(handle);
		handle = FindFirstFileA(joinedPath.c_str(), &result);

		if (handle == INVALID_HANDLE_VALUE) {
			// TODO: LOG_ERROR
			return {};
		}
	}

	std::vector<Path> entries;

	do {
		if (strcmp(result.cFileName, ".") == 0 || strcmp(result.cFileName, "..") == 0) {
			continue;
		}

		Path entry;

		if (result.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			entry.type = PathType::DIRECTORY;
		}
		else if (result.dwFileAttributes & (FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_VIRTUAL)) {
			entry.type = PathType::SPECIAL;
		}
		else {
			entry.type = PathType::FILE;
		}

		entry.name = PathUtils::join(pathName, result.cFileName);

		entries.push_back(std::move(entry));
	}
	while (FindNextFileA(handle, &result));

	CloseHandle(handle);

	return entries;
}

bool OSFileSystem::get_file_stats(const std::string_view& path, FileStats& fileStats) {
	std::string joinedPath = PathUtils::join(m_base, path);
	struct __stat64 buf;

	if (_stat64(joinedPath.c_str(), &buf) < 0) {
		return false;
	}

	if (buf.st_mode & _S_IFREG) {
		fileStats.type = PathType::FILE;
	}
	else if (buf.st_mode & _S_IFDIR) {
		fileStats.type = PathType::DIRECTORY;
	}
	else {
		fileStats.type = PathType::SPECIAL;
	}

	fileStats.size = static_cast<size_t>(buf.st_size);
	fileStats.lastModified = static_cast<uint64_t>(buf.st_mtime);

	return true;
}

#elif defined(OPERATING_SYSTEM_LINUX)

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

OSFileSystem::OSFileSystem(const std::string_view& base)
		: m_base(base) {}

std::vector<Path> OSFileSystem::list(const std::string_view& pathName) {
	std::string joinedPath = PathUtils::join(m_base, pathName);

	DIR* dir = opendir(joinedPath.c_str());

	if (!dir) {
		// TODO: LOG_ERROR failed to open directory
		return {};
	}

	std::vector<Path> entries;
	struct dirent* result;

	while ((result = readdir(dir))) {
		if (strcmp(result->d_name, ".") == 0 || strcmp(result->d_name, "..") == 0) {
			continue;
		}

		Path entry;

		entry.name = PathUtils::join(pathName, result->d_name);

		switch (result->d_type) {
			case DT_DIR:
				entry.type = PathType::DIRECTORY;
				break;
			case DT_REG:
				entry.type = PathType::FILE;
			case DT_UNKNOWN:
			case DT_LNK:
			{
				FileStats s;

				if (!get_file_stats(entry.name, s)) {
					// TODO: LOG_ERROR Failed to stat file
					continue;
				}

				entry.type = s.type;
			}
				break;
			default:
				entry.type = PathType::SPECIAL;
		}

		entries.push_back(std::move(entry));
	}

	closedir(dir);

	return entries;
}

bool OSFileSystem::get_file_stats(const std::string_view& pathName, FileStats& stats) {
	std::string joinedPath = PathUtils::join(m_base, pathName);
	struct stat buf;

	if (stat(joinedPath.c_str(), &buf) < 0) {
		return false;
	}

	if (S_ISREG(buf.st_mode)) {
		stats.type = PathType::FILE;
	}
	else if (S_ISDIR(buf.st_mode)) {
		stats.type = PathType::DIRECTORY;
	}
	else {
		stats.type = PathType::SPECIAL;
	}

	stats.size = static_cast<size_t>(buf.st_size);

	stats.lastModified = buf.st_mtim.tv_sec * 1000000000ull + buf.st_mtim.tv_nsec;

	return true;
}

#endif

