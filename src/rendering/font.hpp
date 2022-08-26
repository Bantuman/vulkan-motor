#pragma once

#include <core/common.hpp>

#include <math/vector2.hpp>
#include <string_view>
#include <memory>
#include <vector>

class Font {
	public:
		struct GlyphPosition {
			Font* font;
			uint32_t glyphIndex;
			uint32_t charIndex;
			Math::Vector2 offset;
		};

		enum class Modifier : uint8_t {
			NONE = 0,
			UPPERCASE = 1,
			LOWERCASE = 2
		};

		explicit Font(std::vector<char>&&, Font* fallback);
		~Font();

		NULL_COPY_AND_ASSIGN(Font);

		float calc_pixel_scale(float pixelSize) const;
		float get_baseline(float scale) const;
		void set_antialiased(bool aa);

		float get_underline_position(float scale) const;
		float get_underline_thickness(float scale) const;

		float get_strikethrough_position(float scale) const;
		float get_strikethrough_thickness(float scale) const;

		bool get_antialiased() const;
		uint32_t get_char_index(uint32_t codepoint) const;

		void get_glyph_positions(const char* text, size_t length, uint8_t fontSize,
				Modifier modifier, std::vector<GlyphPosition>& outBuffer, Math::Vector2& cursor,
				uint32_t stringIndexOffset);

		void get_glyph_positions_with_fallback(const char* text, size_t length, 
				uint8_t fontSize, Modifier modifier, std::vector<GlyphPosition>& outBuffer,
				Math::Vector2& cursor, uint32_t stringIndexOffset);

		void set_pixel_size(uint8_t size);
		void invalidate_shaper();
		void set_fallback(Font* fallback);

		Font* get_font_for_codepoint(uint32_t codepoint);

		struct FT_FaceRec_& get_font_info();
	private:
		struct FT_FaceRec_* m_fontInfo;
		struct hb_font_t* m_hbFont;
		struct hb_buffer_t* m_shapeBuffer;
		std::vector<char> m_fileData;

		Font* m_fallback;
		bool m_antiAliased;
		float m_strikeoutSize;
		float m_strikeoutPosition;
};

struct FontLoader {
	std::shared_ptr<Font> load(const std::string_view& fileName, Font* fallback = nullptr, bool antiAliased = true);
};

