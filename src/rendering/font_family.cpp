#include "font_family.hpp"

#include <algorithm>
#include <cstring>

#include <asset/font_cache.hpp>

#include <file/file_system.hpp>

#include <rendering/font.hpp>

#include <rapidjson/document.h>

static bool get_face_style(const char* name, FontFaceStyle& outStyle);

static std::shared_ptr<Font> get_cjk_fallback_font();

// FontFamily

FontFamily::FontFamily(std::vector<FontFaceTraits>&& normalTraits,
			std::vector<FontFaceTraits>&& italicTraits)
		: m_faceTraits{normalTraits, italicTraits} {}

std::shared_ptr<Font> FontFamily::get_font(uint32_t weight, FontFaceStyle style) const {
	const auto& path = get_font_path(weight, style);

	auto fallback = get_cjk_fallback_font();

	return g_fontCache->get_or_load<FontLoader>(path, path, fallback.get(), style != FontFaceStyle::NOAA);
}

std::shared_ptr<Font> FontFamily::get_default_font() const {
	return get_font(WEIGHT_DEFAULT, FontFaceStyle::NORMAL);
}

const std::string& FontFamily::get_font_path(uint32_t weight, FontFaceStyle style) const {
	auto& traits = m_faceTraits[static_cast<uint8_t>(style)];

	for (auto it = traits.begin(), end = traits.end(); it != end; ++it) {
		if (it == end - 1 || (it + 1)->weight > weight) {
			return it->path;
		}
	}

	if (style == FontFaceStyle::ITALIC) {
		return get_font_path(weight, FontFaceStyle::NORMAL);
	}
	else if (style == FontFaceStyle::NORMAL && weight == WEIGHT_DEFAULT) {
		return m_faceTraits[0].front().path;
	}
	else {
		return get_default_font_path();
	}
}

const std::string& FontFamily::get_default_font_path() const {
	return get_font_path(WEIGHT_DEFAULT, FontFaceStyle::NORMAL);
}

// FontFamilyLoader

std::shared_ptr<FontFamily> FontFamilyLoader::load(const std::string_view& fileName) {
	auto data = g_fileSystem->file_read_bytes(fileName);

	if (data.empty()) {
		return {};
	}

	// ParseInsitu needs null terminator
	data.push_back('\0');

	rapidjson::Document d;
	d.ParseInsitu(data.data());

	if (!d.HasMember("faces")) {
		return {};
	}

	auto& faces = d["faces"];

	if (!faces.IsArray()) {
		return {};
	}

	std::vector<FontFaceTraits> normalTraits;
	std::vector<FontFaceTraits> italicTraits;

	for (auto& fontInfo : faces.GetArray()) {
		if (!fontInfo.IsObject()) {
			return {};
		}

		if (!fontInfo.HasMember("weight") || !fontInfo.HasMember("style")
				|| !fontInfo.HasMember("assetId")) {
			return {};
		}

		auto& vWeight = fontInfo["weight"];
		auto& vStyle = fontInfo["style"];
		auto& vAssetID = fontInfo["assetId"];

		if (!vWeight.IsInt() || !vStyle.IsString() || !vAssetID.IsString()) {
			return {};
		}

		FontFaceStyle style;

		if (!get_face_style(vStyle.GetString(), style)) {
			return {};
		}

		switch (style) {
			case FontFaceStyle::NORMAL:
				normalTraits.emplace_back(FontFaceTraits{
					std::string(vAssetID.GetString()),
					static_cast<uint32_t>(vWeight.GetInt()),
				});
				break;
			case FontFaceStyle::ITALIC:
				italicTraits.emplace_back(FontFaceTraits{
					std::string(vAssetID.GetString()),
					static_cast<uint32_t>(vWeight.GetInt()),
				});
				break;
			default:
				break;
		}
	}

	// Values should already be sorted, however it's always good to make sure
	std::sort(normalTraits.begin(), normalTraits.end(), [](const auto& a, const auto& b) {
		return a.weight < b.weight;
	});

	std::sort(italicTraits.begin(), italicTraits.end(), [](const auto& a, const auto& b) {
		return a.weight < b.weight;
	});

	return std::make_shared<FontFamily>(std::move(normalTraits), std::move(italicTraits));
}

static bool get_face_style(const char* name, FontFaceStyle& outStyle) {
	if (strcmp(name, "normal") == 0) {
		outStyle = FontFaceStyle::NORMAL;
		return true;
	}
	else if (strcmp(name, "italic") == 0) {
		outStyle = FontFaceStyle::ITALIC;
		return true;
	}

	return false;
}

static std::shared_ptr<Font> get_cjk_fallback_font() {
	static constexpr const char* path = "res://fonts/NotoSansCJKjp-Regular.otf";

	return g_fontCache->get_or_load<FontLoader>(path, path);
}

