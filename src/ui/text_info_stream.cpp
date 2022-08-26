#include "rich_text.hpp"

#include <cassert>

#include <core/logging.hpp>

RichText::TextInfoStream::TextInfoStream(const RichText::TextInfo& textInfo,
			const std::vector<Font::GlyphPosition>& glyphPositions, uint8_t uTextSize,
			const Math::Color3& textColor3, const Math::Color3& strokeColor3,
			uint8_t strokeThickness, float strokeTransparency, FontStrokeType strokeType) 
		: m_index(0)
		, m_glyphIndex(0)

		, m_sizeIndex(0)
		, m_colorIndex(0)
		, m_strokeIndex(0)
		, m_underlineIndex(0)
		, m_strikethroughIndex(0)

		, m_textInfo(textInfo)
		, m_glyphPositions(glyphPositions)
		, m_textSize(uTextSize)
		, m_textColor3(textColor3)
		, m_strokeState{strokeColor3, strokeTransparency, strokeThickness, strokeType}

		, m_underline(false)
		, m_strikethrough(false) {}

void RichText::TextInfoStream::advance_to(size_t index) {
	assert(index >= m_glyphIndex);
	m_glyphIndex = index;

	if (m_glyphIndex < m_glyphPositions.size()) {
		advance_to_internal(m_glyphPositions[m_glyphIndex].charIndex);
	}
}

const Font::GlyphPosition& RichText::TextInfoStream::get_glyph_position() const {
	return m_glyphPositions[m_glyphIndex];
}

uint8_t RichText::TextInfoStream::get_text_size() const {
	if (m_sizeIndex < m_textInfo.sizes.size()) {
		auto& state = m_textInfo.sizes[m_sizeIndex];

		if (m_index >= state.begin) {
			return state.value;
		}
	}

	return m_textSize;
}

const Math::Color3& RichText::TextInfoStream::get_text_color3() const {
	if (m_colorIndex < m_textInfo.colors.size()) {
		auto& state = m_textInfo.colors[m_colorIndex];

		if (m_index >= state.begin) {
			return state.value;
		}
	}

	return m_textColor3;
}

const RichText::StrokeState& RichText::TextInfoStream::get_stroke_state() const {
	if (m_strokeIndex < m_textInfo.strokeStates.size()) {
		auto& state = m_textInfo.strokeStates[m_strokeIndex];

		if (m_index >= state.begin) {
			return state.value;
		}
	}

	return m_strokeState;
}

bool RichText::TextInfoStream::has_underline() const {
	return m_underline;
}

bool RichText::TextInfoStream::has_strikethrough() const {
	return m_strikethrough;
}

bool RichText::TextInfoStream::should_start_underline() const {
	return m_underline && (!m_lastUnderline || m_underlineChange);
}

bool RichText::TextInfoStream::should_end_underline() const {
	return (!m_underline && m_lastUnderline)
			|| (m_underline && m_underlineChange && m_lastUnderline);
}

bool RichText::TextInfoStream::should_start_strikethrough() const {
	return m_strikethrough && (!m_lastStrikethrough || m_underlineChange);
}

bool RichText::TextInfoStream::should_end_strikethrough() const {
	return (!m_strikethrough && m_lastStrikethrough)
			|| (m_strikethrough && m_underlineChange && m_lastStrikethrough);
}

void RichText::TextInfoStream::advance_to_internal(size_t index) {
	assert(index >= m_index);
	m_index = index;

	m_underlineChange = false;
	m_lastUnderline = m_underline;
	m_lastStrikethrough = m_strikethrough;

	if (m_sizeIndex < m_textInfo.sizes.size()) {
		const auto& state = m_textInfo.sizes[m_sizeIndex];

		if (m_index >= state.end) {
			++m_sizeIndex;
			m_underlineChange = true;
		}
		else {
			m_underlineChange = m_index >= state.begin;
		}
	}

	if (m_colorIndex < m_textInfo.colors.size()) {
		const auto& state = m_textInfo.colors[m_colorIndex];

		if (m_index >= state.end) {
			++m_colorIndex;
			m_underlineChange = true;
		}
		else {
			m_underlineChange = m_index >= state.begin;
		}
	}

	if (m_strokeIndex < m_textInfo.strokeStates.size()) {
		const auto& state = m_textInfo.strokeStates[m_strokeIndex];

		if (m_index >= state.end) {
			++m_strokeIndex;
		}
	}

	if (m_underlineIndex < m_textInfo.underlines.size()) {
		const auto& state = m_textInfo.underlines[m_underlineIndex];

		if (m_index >= state.end) {
			++m_underlineIndex;
		}
	}

	if (m_strikethroughIndex < m_textInfo.strikethroughs.size()) {
		const auto& state = m_textInfo.strikethroughs[m_strikethroughIndex];

		if (m_index >= state.end) {
			++m_strikethroughIndex;
		}
	}

	if (m_glyphIndex > 0
			&& m_glyphPositions[m_glyphIndex - 1].font != m_glyphPositions[m_glyphIndex].font) {
		m_underlineChange = true;
	}

	m_underline = m_underlineIndex < m_textInfo.underlines.size()
			&& m_index >= m_textInfo.underlines[m_underlineIndex].begin
			&& m_index < m_textInfo.underlines[m_underlineIndex].end;

	m_strikethrough = m_strikethroughIndex < m_textInfo.strikethroughs.size()
			&& m_index >= m_textInfo.strikethroughs[m_strikethroughIndex].begin
			&& m_index < m_textInfo.strikethroughs[m_strikethroughIndex].end;
}

