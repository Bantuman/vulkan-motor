#include "text_gui_object.hpp"

#include <ecs/ecs.hpp>

#include <ui/rich_text.hpp>
#include <rendering/renderer/ui_renderer.hpp>

#include <math/common.hpp>

#include <rendering/text_bitmap.hpp>

#include <core/hashed_string.hpp>

using namespace Game;

namespace {

struct TextLine {
	size_t begin;
	size_t end;
	float width;
};

}

static void calc_line_ranges(Font& font, std::vector<Font::GlyphPosition>& glyphPositions,
		const char* text, float textSize, float lineHeight, const Math::Vector2& absoluteSize,
		bool textWrapped, bool textTruncate, std::vector<TextLine>& lineRanges, bool& textFits);

const std::string& Game::TextGuiObject::get_text() const
{
	return m_text;
}

void TextGuiObject::set_text(ECS::Manager& ecs, Instance& inst, const std::string& text) {
	if (text.compare(m_text) != 0) {
		m_text = text;
		mark_for_redraw(ecs, inst);
	}
}

void TextGuiObject::set_text_color3(ECS::Manager& ecs, const Math::Color3& color) {
	m_textColor3 = color;

	if (is_visible()) {
		auto color4 = color.to_vector4(1.f - m_textTransparency);

		for (auto entity : m_textRects) {
			try_set_rect_color(ecs, entity, color4);
		}
	}
}

void TextGuiObject::set_text_stroke_color3(ECS::Manager& ecs, const Math::Color3& color) {
	m_textStrokeColor3 = color;

	if (is_visible()) {
		auto color4 = color.to_vector4(1.f - m_textStrokeTransparency);

		for (auto entity : m_strokeRects) {
			try_set_rect_color(ecs, entity, color4);
		}
	}
}

void TextGuiObject::set_text_transparency(ECS::Manager& ecs, Instance& inst, float transparency) {
	// FIXME: perhaps ignore if RichText 
	if (transparency == m_textTransparency) {
		return;
	}

	if (transparency == 1.f || m_textTransparency == 1.f) {
		m_textTransparency = transparency;
		// FIXME: doesn't need a full redraw
		mark_for_redraw(ecs, inst);
	}
	else {
		m_textTransparency = transparency;

		auto color4 = m_textColor3.to_vector4(1.f - transparency);

		for (auto entity : m_textRects) {
			try_set_rect_color(ecs, entity, color4);
		}
	}
}

void TextGuiObject::set_text_stroke_transparency(ECS::Manager& ecs, Instance& inst,
		float transparency) {
	// FIXME: perhaps ignore if RichText or UIStroke
	if (transparency == m_textStrokeTransparency) {
		return;
	}

	if (transparency == 1.f || m_textStrokeTransparency == 1.f) {
		m_textStrokeTransparency = transparency;
		// FIXME: doesn't need a full redraw
		mark_for_redraw(ecs, inst);
	}
	else {
		m_textStrokeTransparency = transparency;

		auto color4 = m_textStrokeColor3.to_vector4(1.f - transparency);

		for (auto entity : m_strokeRects) {
			try_set_rect_color(ecs, entity, color4);
		}
	}
}

void TextGuiObject::set_text_size(ECS::Manager& ecs, Instance& inst, uint8_t uTextSize) {
	if (uTextSize < 9) {
		uTextSize = 9;
	}
	else if (uTextSize > 100) {
		uTextSize = 100;
	}

	float textSize = static_cast<float>(uTextSize);

	if (textSize != m_textSize) {
		m_textSize = textSize;
		mark_for_redraw(ecs, inst);
	}
}

void TextGuiObject::set_line_height(ECS::Manager& ecs, Instance& inst, float lineHeight) {
	if (lineHeight != m_lineHeight) {
		m_lineHeight = lineHeight;
		mark_for_redraw(ecs, inst);
	}
}

void TextGuiObject::set_max_visible_graphemes(ECS::Manager& ecs, Instance& inst,
		int32_t maxVisibleGraphemes) {
	if (maxVisibleGraphemes != m_maxVisibleGraphemes) {
		m_maxVisibleGraphemes = maxVisibleGraphemes;
		mark_for_redraw(ecs, inst);
	}
}

void TextGuiObject::set_font(ECS::Manager& ecs, Instance& inst, FontID font) {
	if (font != m_fontID) {
		m_fontID = font;
		mark_for_redraw(ecs, inst);
	}
}

void TextGuiObject::set_text_x_alignment(ECS::Manager& ecs, Instance& inst,
		TextXAlignment textXAlignment) {
	if (textXAlignment != m_textXAlignment) {
		m_textXAlignment = textXAlignment;
		mark_for_redraw(ecs, inst);
	}
}

void TextGuiObject::set_text_y_alignment(ECS::Manager& ecs, Instance& inst,
		TextYAlignment textYAlignment) {
	if (textYAlignment != m_textYAlignment) {
		m_textYAlignment = textYAlignment;
		mark_for_redraw(ecs, inst);
	}
}

void TextGuiObject::set_text_truncate(ECS::Manager& ecs, Instance& inst, TextTruncate trunc) {
	if (trunc != m_textTruncate) {
		m_textTruncate = trunc;
		mark_for_redraw(ecs, inst);
	}
}

void TextGuiObject::set_text_scaled(ECS::Manager& ecs, Instance& inst, bool scaled) {
	if (scaled != m_textScaled) {
		m_textScaled = scaled;
		mark_for_redraw(ecs, inst);
	}
}

void TextGuiObject::set_text_wrapped(ECS::Manager& ecs, Instance& inst, bool wrapped) {
	if (wrapped != m_textWrapped) {
		m_textWrapped = wrapped;
		mark_for_redraw(ecs, inst);
	}
}

void TextGuiObject::set_rich_text(ECS::Manager& ecs, Instance& inst, bool richText) {
	if (richText != m_richText) {
		m_richText = richText;
		mark_for_redraw(ecs, inst);
	}
}

void TextGuiObject::clear(ECS::Manager& ecs) {
	GuiObject::clear(ecs);
	clear_internal(ecs);
}

void TextGuiObject::redraw(ECS::Manager& ecs, RectOrderInfo orderInfo, const ViewportGui& screenGui,
		const Math::Rect* clipRect) {
	orderInfo.overlay = false;
	GuiObject::redraw(ecs, orderInfo, screenGui, clipRect);
	redraw_internal(ecs, orderInfo, screenGui, clipRect);
}

void TextGuiObject::update_rect_transforms(ECS::Manager& ecs, const Math::Vector2& invScreen) {
	static_cast<GuiObject*>(this)->update_rect_transforms(ecs, invScreen);

	for (auto entity : m_textRects) {
		try_update_rect_transform(ecs, entity, invScreen);
	}

	for (auto entity : m_strokeRects) {
		try_update_rect_transform(ecs, entity, invScreen);
	}
}

void TextGuiObject::clear_internal(ECS::Manager& ecs) {
	for (auto entity : m_textRects) {
		try_remove_rect_raw(ecs, entity);
	}

	for (auto entity : m_strokeRects) {
		try_remove_rect_raw(ecs, entity);
	}

	m_textRects.clear();
	m_strokeRects.clear();
}

void TextGuiObject::redraw_internal(ECS::Manager& ecs, RectOrderInfo& orderInfo,
		const ViewportGui& screenGui, const Math::Rect* clipRect) {

	if (m_text.size() == 0 || m_maxVisibleGraphemes == 0 || m_textTransparency == 1.f) {
		clear_internal(ecs);
		return;
	}

	orderInfo.zIndex = get_z_index();
	orderInfo.overlay = true;

	auto pFontFamily = Game::FontIDUtils::get_font_family_by_id(m_fontID);

	if (!pFontFamily) {
		clear_internal(ecs);
		return;
	}

	uint32_t defaultFontWeight;
	FontFaceStyle defaultFontStyle;
	Game::FontIDUtils::get_default_traits_by_id(m_fontID, defaultFontWeight, defaultFontStyle);

	auto pDefaultFont = pFontFamily->get_font(defaultFontWeight, defaultFontStyle);

	if (!pDefaultFont) {
		clear_internal(ecs);
		return;
	}

	auto textSize = m_textScaled ? fminf(100.f, m_absoluteSize.y) : m_textSize;
	textSize += 1.f;
	auto uTextSize = static_cast<uint8_t>(textSize);

	RichText::TextInfo richTextInfo{};
	std::vector<Font::GlyphPosition> glyphPositions;

	if (m_richText) {
		RichText::parse(m_text, m_contentText, *pFontFamily, *pDefaultFont,
				defaultFontWeight, defaultFontStyle, uTextSize, glyphPositions, richTextInfo);

		if (m_contentText.size() == 0) {
			clear_internal(ecs);
			return;
		}
	}
	else {
		m_contentText = m_text;

		Math::Vector2 glyphLineCursor{};
		pDefaultFont->get_glyph_positions_with_fallback(m_text.data(), m_text.size(),
				uTextSize, Font::Modifier::NONE, glyphPositions, glyphLineCursor, 0);
	}



	// FIXME: Input values from UIStroke
	RichText::TextInfoStream infoStream(richTextInfo, glyphPositions, uTextSize, m_textColor3,
			m_textStrokeColor3, 1, m_textStrokeTransparency, FontStrokeType::ROUND);

	orderInfo.overlay = true;

	auto maxGraphemes = m_maxVisibleGraphemes < 0 ? m_contentText.size()
			: static_cast<size_t>(m_maxVisibleGraphemes);

	// Calculate line splits
	std::vector<TextLine> lineRanges;
	calc_line_ranges(*pDefaultFont, glyphPositions, m_contentText.data(), textSize, m_lineHeight,
			m_absoluteSize, m_textWrapped, m_textTruncate == TextTruncate::AT_END, lineRanges,
			m_textFits);

	// TextBounds and TextFits
	float maxWidth = 0.f;

	for (auto& lineRange : lineRanges) {
		if (lineRange.width > maxWidth) {
			maxWidth = lineRange.width;
		}
	}

	m_textBounds = Math::Vector2(maxWidth,
			static_cast<float>(lineRanges.size()) * (textSize - 1.f) * m_lineHeight);

	if (m_textFits && m_textBounds.x > m_absoluteSize.x) {
		m_textFits = false;
	}

	// Draw text
	float scale = pDefaultFont->calc_pixel_scale(textSize);
	float baseline = pDefaultFont->get_baseline(scale);

	float totalHeight = static_cast<float>(lineRanges.size()) * textSize * m_lineHeight;

	uint32_t rectIndex = 0;
	uint32_t strokeIndex = 0;

	for (size_t i = 0; i < lineRanges.size(); ++i) {
		size_t graphemesInLine = lineRanges[i].end - lineRanges[i].begin;

		if (graphemesInLine <= maxGraphemes) {
			maxGraphemes -= graphemesInLine;
		}
		else {
			graphemesInLine = maxGraphemes;
			maxGraphemes = 0;
		}

		draw_text_line(ecs, screenGui.get_inv_size(), orderInfo, clipRect, infoStream,
				lineRanges[i].begin, lineRanges[i].begin + graphemesInLine, i, totalHeight,
				lineRanges[i].width, textSize, baseline, rectIndex, strokeIndex);
	}

	while (rectIndex < m_textRects.size()) {
		m_textRects.pop_back();
	}

	while (strokeIndex < m_strokeRects.size()) {
		m_strokeRects.pop_back();
	}
}

void TextGuiObject::draw_text_line(ECS::Manager& ecs, const Math::Vector2& invScreen,
		RectOrderInfo& orderInfo, const Math::Rect* clipRect, RichText::TextInfoStream& infoStream,
		size_t begin, size_t end, size_t lineNumber, float totalHeight, float textWidth,
		float textSize, float baseline, uint32_t& rectIndex, uint32_t& strokeIndex) {
	if (end <= begin) {
		return;
	}

	RectInstance rect{};
	rect.imageIndex = g_uiRenderer->get_text_image_index();
	rect.samplerIndex = 0;

	Math::Vector2 glyphSize;
	Math::Vector2 glyphOffset;
	Math::Vector4 fontTexLayout;
	bool drawable{};

	// Underline State
	float underlinePosition{};
	float underlineThickness{};
	float underlineMinX{};
	float underlineMaxX{};
	Math::Color3 underlineColor{};

	// Strikethrough State
	float strikethroughPosition{};
	float strikethroughThickness{};
	float strikethroughMinX{};
	float strikethroughMaxX{};
	Math::Color3 strikethroughColor{};

	infoStream.advance_to(begin);
	Math::Vector2 baseTextPos(-infoStream.get_glyph_position().offset.x, baseline);

	switch (m_textXAlignment) {
		case TextXAlignment::RIGHT:
			baseTextPos += Math::Vector2(m_absoluteSize.x - textWidth, 0.f);
			break;
		case TextXAlignment::CENTER:
			baseTextPos += Math::Vector2(0.5f * (m_absoluteSize.x - textWidth), 0.f);
			break;
		default:
			break;
	}

	switch (m_textYAlignment) {
		case TextYAlignment::CENTER:
			baseTextPos += Math::Vector2(0.f, 0.5f * (m_absoluteSize.y - totalHeight)
					+ textSize * m_lineHeight * static_cast<float>(lineNumber));
			break;
		case TextYAlignment::BOTTOM:
			baseTextPos += Math::Vector2(0.f, m_absoluteSize.y - totalHeight
					+ textSize * m_lineHeight * static_cast<float>(lineNumber));
			break;
		default:
			break;
	}

	Math::Transform2D offset(1.f);

	for (size_t i = begin; i < end; ++i) {
		infoStream.advance_to(i);
		auto& gp = infoStream.get_glyph_position();
		auto uTextSize = infoStream.get_text_size();

		g_uiRenderer->get_text_bitmap().get_char_info(*gp.font, gp.glyphIndex, uTextSize,
				fontTexLayout, glyphSize, glyphOffset, drawable);

		// Underline
		if (infoStream.should_end_underline()) {
			underlineMaxX = gp.offset.x + glyphOffset.x;

			RectInstance rectUnderline{};
			rectUnderline.color = underlineColor.to_vector4(1.f - m_textTransparency);

			Math::Vector2 size(underlineMaxX - underlineMinX, underlineThickness);
			auto pos = baseTextPos
					+ Math::Vector2(underlineMinX, underlinePosition)
					+ 0.5f * size;
			pos = 2.f * pos - m_absoluteSize;
			offset.set_translation(pos);
			make_screen_transform(rectUnderline.transform, m_globalTransform * offset, size,
					invScreen);
			emit_rect(ecs, get_or_create_rect(ecs, rectIndex++), rectUnderline, pos, orderInfo, clipRect);
		}

		if (infoStream.should_start_underline()) {
			float scale = gp.font->calc_pixel_scale(static_cast<float>(uTextSize));

			underlinePosition = textSize - baseline + gp.font->get_underline_position(scale);
			underlineThickness = Math::ceil(gp.font->get_underline_thickness(scale));
			underlineMinX = gp.offset.x + glyphOffset.x;
			underlineColor = infoStream.get_text_color3();
		}

		if (infoStream.has_underline()) {
			underlineMaxX = gp.offset.x + glyphOffset.x + glyphSize.x;
		}

		// Strikethrough
		if (infoStream.should_end_strikethrough()) {
			strikethroughMaxX = gp.offset.x + glyphOffset.x;

			RectInstance rectStrikethrough{};
			rectStrikethrough.color = strikethroughColor.to_vector4(1.f - m_textTransparency);

			Math::Vector2 size(strikethroughMaxX - strikethroughMinX, strikethroughThickness);
			auto pos = baseTextPos
					+ Math::Vector2(strikethroughMinX, strikethroughPosition)
					+ 0.5f * size;
			pos = 2.f * pos - m_absoluteSize;
			offset.set_translation(pos);
			make_screen_transform(rectStrikethrough.transform, m_globalTransform * offset, size,
					invScreen);
			emit_rect(ecs, get_or_create_rect(ecs, rectIndex++), rectStrikethrough, pos, orderInfo, clipRect);
		}

		if (infoStream.should_start_strikethrough()) {
			float scale = gp.font->calc_pixel_scale(static_cast<float>(uTextSize));

			strikethroughPosition = -gp.font->get_strikethrough_position(scale);
			strikethroughThickness = Math::ceil(gp.font->get_strikethrough_thickness(scale));
			strikethroughMinX = gp.offset.x + glyphOffset.x;
			strikethroughColor = infoStream.get_text_color3();
		}

		if (infoStream.has_strikethrough()) {
			strikethroughMaxX = gp.offset.x + glyphOffset.x + glyphSize.x;
		}

		if (drawable) {
			if (m_text.compare("Treeview") == 0)
			{
				printf("gp.offset: %f\nglyphSize: %f\n", gp.offset.x, glyphSize.x);
			}
			auto charPos = baseTextPos + gp.offset + glyphOffset;
			//charPos = Math::floor(charPos + Math::Vector2(0.5f)) + 0.5f * glyphSize;
			charPos = Math::floor(charPos) + 0.5f * glyphSize;
			charPos = 2.f * charPos - m_absoluteSize;
		//	charPos.y += 1.f / g_renderContext->get_swapchain_extent().height;
			offset.set_translation(charPos);
			
			auto& strokeState = infoStream.get_stroke_state();

			if (strokeState.transparency < 1.f) {
				Math::Vector2 strokeSize;
				g_uiRenderer->get_text_bitmap().get_stroke_info(*gp.font, gp.glyphIndex, uTextSize,
						strokeState.joins, strokeState.thickness, rect.texLayout, strokeSize);

				rect.color = strokeState.color.to_vector4(1.f - strokeState.transparency);
				make_screen_transform(rect.transform, m_globalTransform * offset, strokeSize,
						invScreen);
				emit_rect(ecs, get_or_create_stroke(ecs, strokeIndex++), rect, charPos, orderInfo,
						clipRect);
			}
			rect.texLayout = fontTexLayout;
			rect.color = infoStream.get_text_color3().to_vector4(1.f - m_textTransparency);
			auto tf = m_globalTransform * offset;
			make_screen_transform(rect.transform, tf, glyphSize,
					invScreen);
			emit_rect(ecs, get_or_create_rect(ecs, rectIndex++), rect, charPos, orderInfo,
					clipRect);
		}
	}
	// Underline
	if (infoStream.has_underline() || infoStream.should_end_underline()) {
		RectInstance rectUnderline{};
		rect.color = underlineColor.to_vector4(1.f - m_textTransparency);

		Math::Vector2 size(underlineMaxX - underlineMinX, underlineThickness);
		auto pos = baseTextPos
				+ Math::Vector2(underlineMinX, underlinePosition)
				+ 0.5f * size;
		pos = 2.f * pos - m_absoluteSize;
		offset.set_translation(pos);
		make_screen_transform(rect.transform, m_globalTransform * offset, size, invScreen);
		emit_rect(ecs, get_or_create_rect(ecs, rectIndex++), rect, pos, orderInfo, clipRect);
	}

	// Strikethrough
	if (infoStream.has_strikethrough() || infoStream.should_end_strikethrough()) {
		RectInstance rectStrikethrough{};
		rectStrikethrough.color = strikethroughColor.to_vector4(1.f - m_textTransparency);

		Math::Vector2 size(strikethroughMaxX - strikethroughMinX, strikethroughThickness);
		auto pos = baseTextPos
				+ Math::Vector2(strikethroughMinX, strikethroughPosition)
				+ 0.5f * size;
		pos = 2.f * pos - m_absoluteSize;
		offset.set_translation(pos);
		make_screen_transform(rectStrikethrough.transform, m_globalTransform * offset, size, invScreen);
		emit_rect(ecs, get_or_create_rect(ecs, rectIndex++), rectStrikethrough, pos, orderInfo, clipRect);
	}
}

ECS::Entity& TextGuiObject::get_or_create_rect(ECS::Manager& ecs, uint32_t index) {
	if (index >= m_textRects.size()) {
		m_textRects.resize(index + 1, ECS::INVALID_ENTITY);
	}
	return m_textRects[index];
}

ECS::Entity& TextGuiObject::get_or_create_stroke(ECS::Manager& ecs, uint32_t index) {
	if (index < m_strokeRects.size()) {
		if (m_strokeRects[index] == ECS::INVALID_ENTITY) {
			m_strokeRects[index] = ecs.create_entity();
		}
	}
	else {
		m_strokeRects.resize(index + 1, ECS::INVALID_ENTITY);
	}

	return m_strokeRects[index];
}

static void calc_line_ranges(Font& font, std::vector<Font::GlyphPosition>& glyphPositions,
		const char* text, float textSize, float lineHeight, const Math::Vector2& absoluteSize,
		bool textWrapped, bool textTruncate, std::vector<TextLine>& lineRanges, bool& textFits) {
	Math::Vector4 fontTexLayout;
	Math::Vector2 glyphSize;
	Math::Vector2 glyphOffset;
	bool drawable;

	auto uTextSize = static_cast<uint8_t>(textSize);

	auto maxNumLines = static_cast<size_t>(absoluteSize.y / (textSize * lineHeight));

	size_t lineStart = 0;
	bool lastBlank = std::isblank(text[glyphPositions[0].charIndex]);
	float lineMin = 0.f;
	float lineMax = 0.f;

	size_t lastWordStartIndex = 0;
	size_t lastWordEndIndex = 0;
	float widthAtLastStart = 0.f;
	float widthAtLastEnd = 0.f;

	textFits = true;

	for (size_t i = 1; i < glyphPositions.size(); ++i) {
		if (textWrapped && lineRanges.size() >= maxNumLines) {
			break;
		}

		char c = text[glyphPositions[i].charIndex];

		if (c == '\n') {
			lineRanges.emplace_back(TextLine{lineStart, i, lineMax - lineMin});
			lineStart = i + 1;
			continue;
		}

		bool blank = std::isblank(c);

		g_uiRenderer->get_text_bitmap().get_char_info(*glyphPositions[i].font,
				glyphPositions[i].glyphIndex, uTextSize,
				fontTexLayout, glyphSize, glyphOffset, drawable);
		float glyphLeft = glyphPositions[i].offset.x + glyphOffset.x;
		float glyphRight = glyphLeft + glyphSize.x;

		float newLineMin = lineMin;
		float newLineMax = lineMax;

		if (text[glyphPositions[i - 1].charIndex] == '\n') {
			lineMin = glyphLeft;
			newLineMin = glyphLeft;
		}

		if (glyphLeft < newLineMin) {
			newLineMin = glyphLeft;
		}

		if (glyphRight > newLineMax) {
			newLineMax = glyphRight;
		}

		float newWidth = newLineMax - newLineMin;

		if (!blank && lastBlank) {
			lastWordStartIndex = i;
			widthAtLastStart = lineMax - lineMin;
		}
		else if (blank && !lastBlank) {
			lastWordEndIndex = i;
			widthAtLastEnd = lineMax - lineMin;
		}

		if (!textWrapped || newWidth <= absoluteSize.x) {
			lineMin = newLineMin;
			lineMax = newLineMax;
		}
		else {
			if (lastWordStartIndex == lastWordEndIndex) {
				// Case A: break in the middle of a word
				//LOG_TEMP("Case A (%c)", c);

				//fwrite(text + lineStart, i - lineStart, 1, stdout);
				//putchar('\n');

				lineRanges.emplace_back(TextLine{lineStart, i, lineMax - lineMin});
				lineStart = i;
				lineMin = glyphLeft;
				lineMax = glyphRight;
			}
			else if (lastWordStartIndex < lastWordEndIndex) {
				// Case B: clean split between words if the iterator is on whitespace
				//LOG_TEMP("Case B (%c)", c);

				//fwrite(text + lineStart, lastWordEndIndex - lineStart, 1, stdout);
				//putchar('\n');

				while (i < glyphPositions.size()
						&& std::isblank(text[glyphPositions[i].glyphIndex])) {
					++i;
				}

				lineRanges.emplace_back(TextLine{lineStart, lastWordEndIndex, widthAtLastEnd});
				lineStart = i;
				lineMin = glyphLeft;
				lineMax = glyphRight;
			}
			else {
				// Case C: even split on whitespace between words
				//LOG_TEMP("Case C (%c)", c);

				//fwrite(text + lineStart, lastWordEndIndex - lineStart, 1, stdout);
				//putchar('\n');

				lineRanges.emplace_back(TextLine{lineStart, lastWordEndIndex, widthAtLastEnd});
				lineStart = lastWordStartIndex;
				lineMin = widthAtLastStart;
				lineMax = newLineMax;
				lastWordEndIndex = lastWordStartIndex;
			}
		}

		lastBlank = blank;
	}

	if (!textWrapped || lineRanges.size() < maxNumLines) {
		lineRanges.emplace_back(TextLine{lineStart, glyphPositions.size(), lineMax - lineMin});
	}
	else {
		textFits = false;
	}

	if (textTruncate && !lineRanges.empty() && (lineRanges.back().end < glyphPositions.size()
				|| lineRanges.back().width > absoluteSize.x)) {
		auto& lastRange = lineRanges.back();

		if (lastRange.width > absoluteSize.x) {
			--lastRange.end;

			g_uiRenderer->get_text_bitmap().get_char_info(*glyphPositions[lastRange.end].font,
					glyphPositions[lastRange.end].glyphIndex, uTextSize,
					fontTexLayout, glyphSize, glyphOffset, drawable);

			float startingPos = glyphPositions[lastRange.end].offset.x + glyphSize.x;

			while (lastRange.end > lastRange.begin && lastRange.width > absoluteSize.x) {
				g_uiRenderer->get_text_bitmap().get_char_info(
						*glyphPositions[lastRange.end - 1].font,
						glyphPositions[lastRange.end - 1].glyphIndex, uTextSize,
						fontTexLayout, glyphSize, glyphOffset, drawable);
				float newEnd = glyphPositions[lastRange.end - 1].offset.x + glyphSize.x;
				float widthDiff = startingPos - newEnd;

				if (lastRange.width - widthDiff <= absoluteSize.x) {
					lastRange.width -= widthDiff;
					break;
				}

				--lastRange.end;
			}
		}

		if (lastRange.end - lastRange.begin > 3) {
			lastRange.end -= 2;
		}

		// Add ellipsis
		glyphPositions[lastRange.end - 1].font = &font;
		glyphPositions[lastRange.end - 1].glyphIndex = font.get_char_index(0x2026);
	}
}
