#pragma once

#include <file/file_system.hpp>

class OSFileSystem final : public FileSystemBackend {
	public:
		OSFileSystem(const std::string_view& base);
		virtual ~OSFileSystem() = default;

		virtual std::vector<Path> list(const std::string_view& pathName) override;
		[[nodiscard]] virtual bool get_file_stats(const std::string_view& path,
				FileStats& stats) override;
		virtual std::string get_file_system_path(const std::string_view& pathName) override;

		virtual std::unique_ptr<InputFile> open_file_read(const std::string_view& path) override;
		virtual std::unique_ptr<OutputFile> open_file_write(const std::string_view& path) override;

	private:
		std::string m_base;
};

