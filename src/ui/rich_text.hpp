#pragma once

#include <string>
#include <vector>

#include <math/color3.hpp>

#include <rendering/font.hpp>
#include <rendering/font_family.hpp>
#include <rendering/font_stroke_type.hpp>

namespace RichText {

template <typename T>
struct StateRange {
	size_t begin;
	size_t end;
	T value;
};

struct EmptyStateRange {
	size_t begin;
	size_t end;
};

struct StrokeState {
	Math::Color3 color;
	float transparency;
	uint8_t thickness;
	FontStrokeType joins;
};

struct TextInfo {
	std::vector<StateRange<uint8_t>> sizes;
	std::vector<StateRange<Math::Color3>> colors;
	std::vector<StateRange<StrokeState>> strokeStates;
	std::vector<EmptyStateRange> underlines;
	std::vector<EmptyStateRange> strikethroughs;
};

class TextInfoStream final {
	public:
		explicit TextInfoStream(const TextInfo&, const std::vector<Font::GlyphPosition>&,
				uint8_t uTextSize, const Math::Color3& textColor3,
				const Math::Color3& strokeColor3, uint8_t strokeThickness,
				float strokeTransparency, FontStrokeType);

		void advance_to(size_t index);

		const Font::GlyphPosition& get_glyph_position() const;

		uint8_t get_text_size() const;
		const Math::Color3& get_text_color3() const;
		const StrokeState& get_stroke_state() const;

		bool has_underline() const;
		bool has_strikethrough() const;

		bool should_start_underline() const;
		bool should_end_underline() const;

		bool should_start_strikethrough() const;
		bool should_end_strikethrough() const;
	private:
		size_t m_index;
		size_t m_glyphIndex;

		size_t m_sizeIndex;
		size_t m_colorIndex;
		size_t m_strokeIndex;
		size_t m_underlineIndex;
		size_t m_strikethroughIndex;

		const TextInfo& m_textInfo;
		const std::vector<Font::GlyphPosition>& m_glyphPositions;

		uint8_t m_textSize;
		Math::Color3 m_textColor3;
		StrokeState m_strokeState;

		bool m_underline;
		bool m_strikethrough;

		bool m_lastUnderline;
		bool m_lastStrikethrough;

		bool m_underlineChange;

		void advance_to_internal(size_t index);
};

bool parse(const std::string& text, std::string& contentText, const FontFamily& fontFamily,
		Font& defaultFont, uint32_t defaultWeight, FontFaceStyle defaultStype,
		uint8_t uTextSize, std::vector<Font::GlyphPosition>& glyphPositions,
		TextInfo& textInfo);

}

