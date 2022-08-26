#include "font_id.hpp"

#include <asset/font_cache.hpp>
#include <asset/font_family_cache.hpp>

#include <core/hashed_string.hpp>
#include <core/logging.hpp>

#include <rendering/font.hpp>
#include <rendering/font_family.hpp>

#include <string_view>
#include <unordered_map>

using namespace Game;

static std::unordered_map<std::string_view, FontID> g_fontNameMap;

static void try_init_font_name_map();

static const char* get_font_file_name_by_id(FontID);
static const char* get_font_family_file_name_by_id(FontID);

std::shared_ptr<Font> Game::FontIDUtils::get_font_by_id(FontID id) {
	const char* fileName = get_font_file_name_by_id(id);
	return g_fontCache->get_or_load<FontLoader>(fileName, fileName);
}

std::shared_ptr<FontFamily> Game::FontIDUtils::get_font_family_by_id(FontID id) {
	auto fileName = get_font_family_file_name_by_id(id);

	return g_fontFamilyCache->get_or_load<FontFamilyLoader>(fileName, fileName);
}

void Game::FontIDUtils::get_default_traits_by_id(FontID id, uint32_t& weight,
		FontFaceStyle& style) {
	switch (id) {
		case FontID::SOURCE_SANS_ITALIC:
			weight = FontFamily::WEIGHT_DEFAULT;
			style = FontFaceStyle::ITALIC;
			break;
		case FontID::ARIAL_BOLD:
		case FontID::SOURCE_SANS_BOLD:
			weight = FontFamily::WEIGHT_BOLD;
			style = FontFaceStyle::NORMAL;
			break;
		case FontID::SOURCE_SANS_SEMI_BOLD:
		//case FontID::GOTHAM_SEMI_BOLD:
			weight = FontFamily::WEIGHT_SEMI_BOLD;
			style = FontFaceStyle::NORMAL;
			break;
		//case FontID::GOTHAM_BLACK:
		//	weight = FontFamily::WEIGHT_BLACK;
		//	style = FontFaceStyle::NORMAL;
		//	break;
		case FontID::SOURCE_SANS_LIGHT:
			weight = FontFamily::WEIGHT_LIGHT;
			style = FontFaceStyle::NORMAL;
			break;
		default:
			weight = FontFamily::WEIGHT_DEFAULT;
			style = FontFaceStyle::NORMAL;
	}
}	

Game::FontID Game::FontIDUtils::get_font_id_by_name(const char* name, size_t size) {
	try_init_font_name_map();

	if (auto it = g_fontNameMap.find(std::string_view(name, size)); it != g_fontNameMap.end()) {
		return it->second;
	}

	return FontID::INVALID_FONT;
}

static void try_init_font_name_map() {
	if (g_fontNameMap.empty()) {
		g_fontNameMap.emplace(std::make_pair(std::string_view("Arial"), FontID::ARIAL));
		g_fontNameMap.emplace(std::make_pair(std::string_view("ArialBold"),
					FontID::ARIAL_BOLD));
		g_fontNameMap.emplace(std::make_pair(std::string_view("SourceSans"),
					FontID::SOURCE_SANS));
		g_fontNameMap.emplace(std::make_pair(std::string_view("SourceSansBold"),
					FontID::SOURCE_SANS_BOLD));
		g_fontNameMap.emplace(std::make_pair(std::string_view("SourceSansSemibold"),
					FontID::SOURCE_SANS_SEMI_BOLD));
		g_fontNameMap.emplace(std::make_pair(std::string_view("SourceSansLight"),
					FontID::SOURCE_SANS_LIGHT));
		g_fontNameMap.emplace(std::make_pair(std::string_view("SourceSansItalic"),
					FontID::SOURCE_SANS_ITALIC));
		g_fontNameMap.emplace(std::make_pair(std::string_view("MSGothic"),
			FontID::MSGOTHIC));
	}
}

static const char* get_font_file_name_by_id(FontID id) {
	switch (id) {
		case FontID::ARIAL:
			return "res://fonts/arial.ttf";
		case FontID::ARIAL_BOLD:
			return "res://fonts/arialbd.ttf";
		case FontID::SOURCE_SANS:
			return "res://fonts/SourceSansPro-Regular.ttf";
		case FontID::SOURCE_SANS_BOLD:
			return "res://fonts/SourceSansPro-Bold.ttf";
		case FontID::SOURCE_SANS_LIGHT:
			return "res://fonts/SourceSansPro-Light.ttf";
		case FontID::SOURCE_SANS_ITALIC:
			return "res://fonts/SourceSansPro-It.ttf";
		case FontID::SOURCE_SANS_SEMI_BOLD:
			return "res://fonts/SourceSansPro-Semibold.ttf";
		case FontID::MSGOTHIC:
			return "res://fonts/MS-UIGothic-02.ttf";
		default:
			LOG_ERROR("FONT", "INVALID FONT %d", id);
			return "";
	}
}

static const char* get_font_family_file_name_by_id(FontID id) {
	switch (id) {
		case FontID::ARIAL:
		case FontID::ARIAL_BOLD:
			return "res://fonts/families/Arial.json";
		case FontID::SOURCE_SANS:
		case FontID::SOURCE_SANS_BOLD:
		case FontID::SOURCE_SANS_LIGHT:
		case FontID::SOURCE_SANS_ITALIC:
		case FontID::SOURCE_SANS_SEMI_BOLD:
			return "res://fonts/families/SourceSansPro.json";
		case FontID::MSGOTHIC:
			return "res://fonts/families/MSGothic.json";
		default:
			LOG_ERROR("FONT", "INVALID FONT %d", id);
			return "";
	}
}

