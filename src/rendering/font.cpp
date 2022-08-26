#include "font.hpp"

#include <atomic>
#include <algorithm>

#include <cctype>
#include <cstring>

#include <core/logging.hpp>
#include <core/utf8.hpp>

#include <file/file_system.hpp>

#include <rendering/freetype.hpp>

#include <hb-ft.h>

#include FT_ADVANCES_H
#include FT_TRUETYPE_TABLES_H

#define NO_TEXT_AA

static std::atomic_size_t g_instanceCount{0};

static void freetype_init();
static void freetype_deinit();

static void force_ucs2_charmap(FT_Face);

// Font

Font::Font(std::vector<char>&& fileData, Font* fallback)
		: m_fileData(fileData)
		, m_fallback(fallback)
		, m_strikeoutSize(1.f)
		, m_strikeoutPosition(0.f) {
	freetype_init();

	FT_New_Memory_Face(g_freeType, reinterpret_cast<unsigned char*>(m_fileData.data()),
			m_fileData.size(), 0, &m_fontInfo);
	force_ucs2_charmap(m_fontInfo);

	TT_OS2* pOS2Table = reinterpret_cast<TT_OS2*>(FT_Get_Sfnt_Table(m_fontInfo, FT_SFNT_OS2));

	if (pOS2Table) {
		m_strikeoutSize = static_cast<float>(pOS2Table->yStrikeoutSize);
		m_strikeoutPosition = static_cast<float>(pOS2Table->yStrikeoutPosition);
	}

	m_hbFont = hb_ft_font_create(m_fontInfo, NULL);
	m_shapeBuffer = hb_buffer_create();
}

Font::~Font() {
	hb_buffer_destroy(m_shapeBuffer);
	hb_font_destroy(m_hbFont);
	FT_Done_Face(m_fontInfo);
	freetype_deinit();
}

float Font::calc_pixel_scale(float pixelSize) const {
	return pixelSize / static_cast<float>(m_fontInfo->ascender - m_fontInfo->descender);
}

float Font::get_baseline(float scale) const {
	return static_cast<float>(m_fontInfo->ascender) * scale;
}

void Font::set_antialiased(bool aa)
{
	m_antiAliased = aa;
}

float Font::get_underline_position(float scale) const {
	return static_cast<float>(m_fontInfo->underline_position) * scale;
}

float Font::get_underline_thickness(float scale) const {
	return static_cast<float>(m_fontInfo->underline_thickness) * scale;
}

float Font::get_strikethrough_position(float scale) const {
	return m_strikeoutPosition * scale;
}

float Font::get_strikethrough_thickness(float scale) const {
	return m_strikeoutSize * scale; 
}

bool Font::get_antialiased() const
{
	return m_antiAliased;
}

uint32_t Font::get_char_index(uint32_t codepoint) const {
	return static_cast<uint32_t>(FT_Get_Char_Index(m_fontInfo, codepoint));
}

void Font::set_pixel_size(uint8_t size) {
	FT_Size_RequestRec sr{};
	sr.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
	sr.height = static_cast<FT_Long>(size) * 64;
	FT_Request_Size(m_fontInfo, &sr);
}

void Font::get_glyph_positions(const char* text, size_t length, uint8_t fontSize,
		Font::Modifier modifier, std::vector<GlyphPosition>& outBuffer, Math::Vector2& cursor,
		uint32_t stringIndexOffset) {
	set_pixel_size(fontSize);
	invalidate_shaper();

	hb_buffer_reset(m_shapeBuffer);

	hb_buffer_set_direction(m_shapeBuffer, HB_DIRECTION_LTR);
	hb_buffer_set_script(m_shapeBuffer, HB_SCRIPT_LATIN);
	hb_buffer_set_language(m_shapeBuffer, hb_language_from_string("en", -1));

	switch (modifier) {
		case Modifier::UPPERCASE:
		{
			char* buffer = reinterpret_cast<char*>(malloc(length * sizeof(char)));
			memcpy(buffer, text, length * sizeof(char));

			std::transform(buffer, buffer + length, buffer, [](char c) {
				return std::toupper(c);
			});

			hb_buffer_add_utf8(m_shapeBuffer, buffer, length, 0, length);
			free(buffer);
		}
			break;
		case Modifier::LOWERCASE:
		{
			char* buffer = reinterpret_cast<char*>(malloc(length * sizeof(char)));
			memcpy(buffer, text, length * sizeof(char));

			std::transform(buffer, buffer + length, buffer, [](char c) {
				return std::tolower(c);
			});

			hb_buffer_add_utf8(m_shapeBuffer, buffer, length, 0, length);
			free(buffer);
		}
			break;
		default:
			hb_buffer_add_utf8(m_shapeBuffer, text, length, 0, length);
	}

	hb_shape(m_hbFont, m_shapeBuffer, nullptr, 0);

	unsigned glyphCount;
	auto* glyphInfo = hb_buffer_get_glyph_infos(m_shapeBuffer, &glyphCount);
	auto* glyphPos = hb_buffer_get_glyph_positions(m_shapeBuffer, &glyphCount);

	for (unsigned i = 0; i < glyphCount; ++i) {
		outBuffer.emplace_back(GlyphPosition{
			this,
			static_cast<uint32_t>(glyphInfo[i].codepoint),
			stringIndexOffset + glyphInfo[i].cluster,
			cursor + Math::Vector2(static_cast<float>(glyphPos[i].x_offset) / 64.f,
					static_cast<float>(glyphPos[i].y_offset) / 64.f)
		});

		cursor += Math::Vector2(static_cast<float>(glyphPos[i].x_advance) / 64.f,
				static_cast<float>(glyphPos[i].y_advance) / 64.f);
	}
}

void Font::get_glyph_positions_with_fallback(const char* text, size_t length, 
		uint8_t fontSize, Font::Modifier modifier, std::vector<GlyphPosition>& outBuffer,
		Math::Vector2& cursor, uint32_t stringIndexOffset) {
	UTF8::CodePointStream cpStream(text, text + length);
	auto pCurrFont = this;
	const char* lastFontChangeStart = text;

	while (cpStream.good()) {
		auto currPos = cpStream.get_position();
		auto codepoint = cpStream.get();
		auto pNextFont = get_font_for_codepoint(codepoint);

		if (pNextFont != pCurrFont) {
			pCurrFont->get_glyph_positions(lastFontChangeStart,
					(currPos - lastFontChangeStart), fontSize, modifier, outBuffer, cursor,
					static_cast<uint32_t>(lastFontChangeStart - text) + stringIndexOffset);
			lastFontChangeStart = currPos;
			pCurrFont = pNextFont;
		}
	}

	pCurrFont->get_glyph_positions(lastFontChangeStart, length - (lastFontChangeStart - text),
			fontSize, modifier, outBuffer, cursor,
			static_cast<uint32_t>(lastFontChangeStart - text) + stringIndexOffset);
}

void Font::invalidate_shaper() {
	hb_ft_font_changed(m_hbFont);
}

void Font::set_fallback(Font* fallback) {
	m_fallback = fallback;
}

Font* Font::get_font_for_codepoint(uint32_t codepoint) {
	if (codepoint <= 0x7Fu) {
		return this;
	}

	Font* res = this;

	while (res) {
		auto glyphIndex = FT_Get_Char_Index(res->m_fontInfo, codepoint);

		if (glyphIndex != 0) {
			return res;
		}

		res = res->m_fallback;
	}

	return this;
}

FT_FaceRec& Font::get_font_info() {
	return *m_fontInfo;
}

// FontLoader

std::shared_ptr<Font> FontLoader::load(const std::string_view& fileName, Font* fallback, bool antiAliased) {
	auto data = g_fileSystem->file_read_bytes(fileName);

	if (data.empty()) {
		return {};
	}

	auto s = std::make_shared<Font>(std::move(data), fallback);
#ifndef NO_TEXT_AA
	s->set_antialiased(antiAliased);
#else
	s->set_antialiased(false);
#endif
	return s;
}

// FreeType

static void freetype_init() {
	if (g_instanceCount.fetch_add(1, std::memory_order_relaxed) == 0) {
		FT_Init_FreeType(&g_freeType);
	}
}

static void freetype_deinit() {
	if (g_instanceCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
		FT_Done_FreeType(g_freeType);
	}
}

static void force_ucs2_charmap(FT_Face font) {
	for (int i = 0; i < font->num_charmaps; ++i) {
		auto* charmap = font->charmaps[i];

		if ((charmap->platform_id == 0 && charmap->encoding_id == 3)
				|| (charmap->platform_id == 3 && charmap->encoding_id == 1)) {
			FT_Set_Charmap(font, charmap);
			return;
		}
	}
}

