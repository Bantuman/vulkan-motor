#pragma once

#include <rendering/font_family.hpp>

namespace Game {

enum class FontID : uint8_t {
	ARIAL = 0,
	ARIAL_BOLD = 1,
	SOURCE_SANS = 2,
	SOURCE_SANS_BOLD = 3,
	SOURCE_SANS_SEMI_BOLD = 4,
	SOURCE_SANS_LIGHT = 5,
	SOURCE_SANS_ITALIC = 6,
	MSGOTHIC = 7,

	INVALID_FONT
};

}

namespace Game::FontIDUtils {

std::shared_ptr<Font> get_font_by_id(Game::FontID);
std::shared_ptr<FontFamily> get_font_family_by_id(Game::FontID);

void get_default_traits_by_id(Game::FontID, uint32_t& weight, FontFaceStyle& style);

Game::FontID get_font_id_by_name(const char* name, size_t size);

}

