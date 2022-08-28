#include "web_file_system.hpp"

#include <file.hpp>
#include <net/web.hpp>

#include <cassert>
#include <cstdio>

#include <regex>

#include <rapidjson/document.h>
#include <zlib.h>

static void write_asset_file(const std::string_view& fileName, std::vector<char>& fileData);

static size_t get_max_compressed_size(size_t srcSize);
static int uncompress_data(void* srcBuffer, size_t srcSize, void* dstBuffer, size_t dstSize);

std::vector<Path> WebFileSystem::list(const std::string_view& pathName) {
	(void)pathName;

	std::vector<Path> res;

	for (auto& [id, fileName] : m_manifest) {
		res.emplace_back(Path{ fileName, PathType::FILE });
	}

	return res;
}

bool WebFileSystem::get_file_stats(const std::string_view& path, FileStats& stats) {
	return g_fileSystem->get_backend("assetcache")->get_file_stats(path, stats);
}

std::string WebFileSystem::get_file_system_path(const std::string_view& pathName) {
	return g_fileSystem->get_backend("assetcache")->get_file_system_path(pathName);
}

std::unique_ptr<InputFile> WebFileSystem::open_file_read(const std::string_view& path) {
	auto fileName = get_file_name(path);

	if (fileName.empty()) {
		std::string requestURL = path.data();

		const char* headers[] = { "Accept: application/json" };

		Web::Request req{};
		req.url = requestURL.c_str();
		req.method = Web::RequestMethod::GET;
		req.pHeaders = headers;
		req.headerCount = 1;

		if (Web::Response res{}; Web::invoke_request(req, res)) {
			rapidjson::Document d;
			fwrite(res.body.data(), 1, res.body.size(), stdout);
			putchar('\n');

			d.Parse(res.body.data(), res.body.size());

			if (!d.HasMember("location")) {
				return nullptr;
			}

			const char* cdnUrl = d["location"].GetString();
			std::cmatch m;

			req.url = cdnUrl;
			req.headerCount = 0;

			if (!Web::invoke_request(req, res)) {
				return nullptr;
			}

			write_asset_file(m[1].str(), res.body);
			set_file_name(path, m[1].str());
			fileName = m[1].str();
		}
		else {
			return nullptr;
		}
	}

	return g_fileSystem->open_file_read(std::string("assetcache://") + std::string(fileName));
}

std::unique_ptr<OutputFile> WebFileSystem::open_file_write(const std::string_view&) {
	return nullptr;
}

std::string WebFileSystem::get_file_name(const std::string_view& assetID) {
	if (!m_initialized) {
		read_manifest();
		m_initialized = true;
	}

	if (auto it = m_manifest.find(std::string(assetID)); it != m_manifest.end()) {
		return it->second;
	}

	return "";
}

void WebFileSystem::set_file_name(const std::string_view& assetID,
	const std::string_view& fileName) {
	assert(m_initialized);

	m_manifest[std::string(assetID)] = std::string(fileName);
	write_manifest();
}

void WebFileSystem::read_manifest() {
	auto data = g_fileSystem->file_read_bytes("assetcache://manifest.csv");

	const char* tokenStart = data.data();
	std::string_view svID;
	std::string_view svFileName;

	for (const char* c = data.data(), *end = data.data() + data.size(); c != end; ++c) {
		switch (*c) {
		case ',':
			svID = std::string_view(tokenStart, c - tokenStart);
			tokenStart = c + 1;
			break;
		case '\n':
			svFileName = std::string_view(tokenStart, c - tokenStart);
			tokenStart = c + 1;

			m_manifest[std::string(svID)] = std::string(svFileName);
			break;
		}
	}
}

void WebFileSystem::write_manifest() {
	auto outFile = g_fileSystem->open_file_write("assetcache://manifest.csv");

	for (auto& [id, fileName] : m_manifest) {
		outFile->write(id.data(), id.size());
		outFile->write(",", 1);
		outFile->write(fileName.data(), fileName.size());
		outFile->write("\n", 1);
	}
}

static void write_asset_file(const std::string_view& fileName, std::vector<char>& fileData) {
	if (fileData.size() < 2) {
		return;
	}

	uint16_t magicNumber = *reinterpret_cast<uint16_t*>(fileData.data());

	// If this file is a gzip
	if (magicNumber == 0x8B1F) {
		size_t maxUncompressed = get_max_compressed_size(fileData.size());
		std::vector<char> uncompressedBuffer;
		uncompressedBuffer.resize(maxUncompressed);

		int res = uncompress_data(fileData.data(), fileData.size(), uncompressedBuffer.data(),
			uncompressedBuffer.size());

		if (res > 0) {
			auto outFile = g_fileSystem->open_file_write(std::string("assetcache://")
				+ std::string(fileName));
			outFile->write(uncompressedBuffer.data(), uncompressedBuffer.size());

			return;
		}
	}

	auto outFile = g_fileSystem->open_file_write(std::string("assetcache://")
		+ std::string(fileName));
	outFile->write(fileData.data(), fileData.size());
}

static size_t get_max_compressed_size(size_t srcSize) {
	size_t n16kBlocks = (srcSize + 16383) / 16384;
	return srcSize + 6 + (n16kBlocks * 5);
}

static int uncompress_data(void* srcBuffer, size_t srcSize, void* dstBuffer, size_t dstSize) {
	z_stream zInfo{};
	zInfo.total_in = srcSize;
	zInfo.avail_in = srcSize;
	zInfo.total_out = dstSize;
	zInfo.avail_out = dstSize;
	zInfo.next_in = reinterpret_cast<unsigned char*>(srcBuffer);
	zInfo.next_out = reinterpret_cast<unsigned char*>(dstBuffer);

	int nErr = inflateInit2(&zInfo, 15 + 32);
	int nRet = -1;

	if (nErr == Z_OK) {
		nErr = inflate(&zInfo, Z_FINISH);

		if (nErr == Z_STREAM_END) {
			nRet = zInfo.total_out;
		}
	}

	inflateEnd(&zInfo);

	return nRet;
}

