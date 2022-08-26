#include "file_system.hpp"

#include <path_utils.hpp>
#include <os_file_system.hpp>
#include <file.hpp>

#define MSVC

FileSystem::FileSystem() {
	add_backend("file", std::make_unique<OSFileSystem>("."));
	add_backend("res", std::make_unique<OSFileSystem>("../res"));
	add_backend("shaders", std::make_unique<OSFileSystem>("../shaders"));
}

void FileSystem::add_backend(const std::string_view& scheme,
		std::unique_ptr<FileSystemBackend> backend) {
	backend->set_scheme(scheme);
	m_backends[std::string(scheme)] = std::move(backend);
}

std::vector<Path> FileSystem::walk(const std::string_view& path) {
	auto [scheme, pathName] = PathUtils::split_scheme(path);
	FileSystemBackend* backend = get_backend(scheme);

	if (!backend) {
		return {};
	}

	return backend->walk(pathName);
}

std::vector<Path> FileSystem::list(const std::string_view& path) {
	auto [scheme, pathName] = PathUtils::split_scheme(path);
	FileSystemBackend* backend = get_backend(scheme);

	if (!backend) {
		return {};
	}
	
	return backend->list(pathName);
}

bool FileSystem::get_file_stats(const std::string_view& path, FileStats& fileStats) {
	auto [scheme, pathName] = PathUtils::split_scheme(path);
	FileSystemBackend* backend = get_backend(scheme);

	if (!backend) {
		return false;
	}

	return backend->get_file_stats(pathName, fileStats);
}

std::unique_ptr<InputFile> FileSystem::open_file_read(const std::string_view& path) {
	auto [scheme, pathName] = PathUtils::split_scheme(path);
	FileSystemBackend* backend = get_backend(scheme);

	if (!backend) {
		return {};
	}

	return backend->open_file_read(pathName);
}

std::unique_ptr<OutputFile> FileSystem::open_file_write(const std::string_view& path) {
	auto [scheme, pathName] = PathUtils::split_scheme(path);
	FileSystemBackend* backend = get_backend(scheme);

	if (!backend) {
		return {};
	}

	return backend->open_file_write(pathName);
}

std::vector<char> FileSystem::file_read_bytes(const std::string_view& path) {
	auto file = open_file_read(path);

	if (!file) {
		return {};
	}

	size_t size = file->get_size();
	std::vector<char> res;
	res.resize(size);

	file->read(res.data(), size);

	return res;
}

std::string FileSystem::get_file_system_path(const std::string_view& path) {
	auto [scheme, pathName] = PathUtils::split_scheme(path);
	FileSystemBackend* backend = get_backend(scheme);

	if (!backend) {
		return "";
	}

	return backend->get_file_system_path(pathName);
}

FileSystemBackend* FileSystem::get_backend(const std::string_view& scheme) {
	std::unordered_map<std::string, std::unique_ptr<FileSystemBackend>>::iterator it;

	if (scheme.empty()) {
		it = m_backends.find("file");
	}
	else {
		it = m_backends.find(std::string(scheme));
	}

	if (it != m_backends.end()) {
		return it->second.get();
	}

	return nullptr;
}

// FileSystemBackend

std::vector<Path> FileSystemBackend::walk(const std::string_view& path) {
	std::vector<std::vector<Path>> entryStack;
	std::vector<Path> result;

	entryStack.push_back(std::move(list(path)));

	while (!entryStack.empty()) {
		auto entries = entryStack.back();
		entryStack.pop_back();

		for (auto& entry : entries) {
			switch (entry.type) {
				case PathType::DIRECTORY:
					entryStack.push_back(std::move(list(entry.name)));
					break;
				default:
					result.push_back(std::move(entry));
			}
		}
	}

	return result;
}

std::string FileSystemBackend::get_file_system_path(const std::string_view&) {
	return "";
}

void FileSystemBackend::set_scheme(const std::string_view& scheme) {
	m_scheme = std::string(scheme);
}

