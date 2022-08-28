#pragma once

#include <file/file_system.hpp>

#include <unordered_map>

class WebFileSystem final : public FileSystemBackend {
public:
	explicit WebFileSystem() = default;
	virtual ~WebFileSystem() = default;

	virtual std::vector<Path> list(const std::string_view& pathName) override;
	[[nodiscard]] virtual bool get_file_stats(const std::string_view& path,
		FileStats& stats) override;
	virtual std::string get_file_system_path(const std::string_view& pathName) override;

	virtual std::unique_ptr<InputFile> open_file_read(const std::string_view& path) override;
	virtual std::unique_ptr<OutputFile> open_file_write(const std::string_view& path) override;
private:
	std::unordered_map<std::string, std::string> m_manifest;
	bool m_initialized = false;

	std::string get_file_name(const std::string_view& assetID);
	void set_file_name(const std::string_view& assetID, const std::string_view& fileName);

	void read_manifest();
	void write_manifest();
};
