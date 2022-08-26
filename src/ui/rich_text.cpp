#include "rich_text.hpp"

#include <cstdint>
#include <charconv>
#include <type_traits>

#include <ui/font_id.hpp>

#include <rendering/font.hpp>
#include <rendering/font_family.hpp>

#include <math/common.hpp>

static constexpr const uint8_t FLAG_HAS_FACE_CHANGE		= 0b1;
static constexpr const uint8_t FLAG_HAS_SIZE_CHANGE		= 0b10;
static constexpr const uint8_t FLAG_HAS_COLOR_CHANGE	= 0b100;
static constexpr const uint8_t FLAG_HAS_MODIFIER		= 0b1000;

struct FontState {
	const FontFamily* family;
	Font* font;
	uint32_t defaultWeight;
	FontFaceStyle defaultStyle;
	Font::Modifier modifier;
	uint8_t size;
	bool bold;
	bool italic;
};

class RichTextParser {
	public:
		explicit RichTextParser(RichText::TextInfo& textInfo, const FontFamily& fontFamily,
					Font& font, uint32_t defaultWeight, FontFaceStyle defaultStyle,
					uint8_t uTextSize, std::vector<Font::GlyphPosition>& glyphPositions,
					const char* begin, const char* end)
				: m_currPos(begin)
				, m_end(end)
				, m_error(false)
				, m_textInfo(textInfo)
				, m_glyphPositions(glyphPositions)
				, m_textIndex(0) {
			m_fontStates.emplace_back(FontState{&fontFamily, &font, defaultWeight, defaultStyle,
					Font::Modifier::NONE, uTextSize, false, false});
		}

		void parse();

		bool has_error() const {
			return m_error;
		}

		std::string_view get_content_text() const {
			return std::string_view(m_output.data(), m_output.size());
		}
	private:
		const char* m_currPos;
		const char* m_end;

		bool m_error;

		std::vector<char> m_output;

		RichText::TextInfo& m_textInfo;

		std::vector<Font::GlyphPosition>& m_glyphPositions;

		std::vector<FontState> m_fontStates;
		std::vector<uint8_t> m_fontModificationFlags;

		Math::Vector2 m_textCursor;
		size_t m_textIndex;

		void parse_open_bracket();
		void parse_end_tag();

		void parse_comment();
		
		void parse_font();
		void parse_font_end();

		void parse_italic();
		void parse_italic_end();

		void parse_font_attributes();

		void parse_b_tag();
		void parse_s_tag();
		void parse_u_tag();

		void parse_b_end_tag();
		void parse_s_end_tag();
		void parse_u_end_tag();

		void parse_stroke();
		void parse_stroke_end();

		void parse_stroke_attributes();
		void parse_stroke_color();
		void parse_stroke_joins();
		void parse_stroke_t_attributes();
		void parse_stroke_thickness();
		void parse_stroke_transparency();

		void parse_line_break();

		bool parse_color(uint32_t&);
		bool parse_color_hex(uint32_t&);
		bool parse_color_rgb(uint32_t&);

		void parse_font_color();
		void parse_font_size();
		void parse_font_face();

		template <typename T>
		bool parse_numeric_with_stop(T&, char);

		char get();
		char peek() const;

		bool has_next() const;
		bool has_next_n(size_t) const;
		void advance();
		void advance_n(size_t);

		bool consume_char(char);
		template <size_t Size>
		bool consume_word(const char*);

		void raise_error(const char* msg);

		void flag_uppercase_start();
		void flag_uppercase_end();

		void commit_font_state();
};

bool RichText::parse(const std::string& text, std::string& contentText,
		const FontFamily& fontFamily, Font& defaultFont, uint32_t defaultWeight,
		FontFaceStyle defaultStyle, uint8_t uTextSize,
		std::vector<Font::GlyphPosition>& glyphPositions, RichText::TextInfo& textInfo) {
	RichTextParser parser(textInfo, fontFamily, defaultFont, defaultWeight, defaultStyle,
			uTextSize, glyphPositions, text.data(), text.data() + text.size());
	parser.parse();

	contentText = std::string(parser.get_content_text());
	return !parser.has_error();
}

void RichTextParser::parse() {
	for (;;) {
		if (!has_next()) {
			commit_font_state();
			return;
		}

		char c = get();

		if (c == '<') {
			parse_open_bracket();
		}
		else {
			m_output.push_back(c);
		}
	}
}

void RichTextParser::parse_open_bracket() {
	if (!has_next()) {
		raise_error("open_bracket: does not have next");
		return;
	}

	char c = get();

	switch (c) {
		case '!':
			parse_comment();
			break;
		case '/':
			parse_end_tag();
			break;
		case 'b':
			parse_b_tag();
			break;
		case 'f':
			parse_font();
			break;
		case 'i':
			parse_italic();
			break;
		case 's':
			parse_s_tag();
			break;
		case 'u':
			parse_u_tag();
			break;
		default:
			raise_error("open_bracket: did not get expected char");
	}
}

void RichTextParser::parse_end_tag() {
	if (!has_next()) {
		raise_error("end_tag: does not have next");
		return;
	}

	char c = get();

	switch (c) {
		case 'b':
			parse_b_end_tag();
			break;
		case 'f':
			parse_font_end();
			break;
		case 'i':
			parse_italic_end();
			break;
		case 's':
			parse_s_end_tag();
			break;
		case 'u':
			parse_u_end_tag();
			break;
		default:
			raise_error("end_tag: did not get expected char");
	}
}

void RichTextParser::parse_comment() {
	if (!consume_char('-')) {
		return;
	}

	if (!consume_char('-')) {
		return;
	}

	for (;;) {
		if (!has_next()) {
			raise_error("comment: does not have next");
			return;
		}

		char c = get();

		if (c == '-') {
			if (!consume_char('-')) {
				return;
			}

			if (!consume_char('>')) {
				return;
			}

			break;
		}
	}
}

void RichTextParser::parse_font() {
	if (!consume_word<3>("ont")) {
		return;
	}

	m_fontModificationFlags.push_back(0);

	parse_font_attributes();
}

void RichTextParser::parse_font_end() {
	consume_word<4>("ont>");

	auto flags = m_fontModificationFlags.back();
	m_fontModificationFlags.pop_back();

	if (flags & FLAG_HAS_FACE_CHANGE) {
		commit_font_state();
		m_fontStates.pop_back();
	}

	if (flags & FLAG_HAS_SIZE_CHANGE) {
		m_textInfo.sizes.back().end = m_output.size();
	}

	if (flags & FLAG_HAS_COLOR_CHANGE) {
		m_textInfo.colors.back().end = m_output.size();
	}
}

void RichTextParser::parse_italic() {
	if (!consume_char('>')) {
		return;
	}

	commit_font_state();
	auto newState = m_fontStates.back();
	newState.italic = true;
	newState.font = newState.family->get_font(newState.bold ? FontFamily::WEIGHT_BOLD
			: newState.defaultWeight, FontFaceStyle::ITALIC).get();

	m_fontStates.emplace_back(std::move(newState));
}

void RichTextParser::parse_italic_end() {
	if (!consume_char('>')) {
		return;
	}

	commit_font_state();
	m_fontStates.pop_back();
}

void RichTextParser::parse_font_attributes() {
	for (;;) {
		if (!has_next()) {
			raise_error("font_attributes: does not have next");
			return;
		}

		char c = get();

		switch (c) {
			case 'c':
				parse_font_color();
				break;
			case 'f':
				parse_font_face();
				break;
			case 's':
				parse_font_size();
				break;
			case ' ':
				break;
			case '>':
				return;
			default:
				raise_error("font_attributes: invalid char");
		}
	}
}

void RichTextParser::parse_b_tag() {
	if (!has_next()) {
		raise_error("b_tag: does not have next");
		return;
	}

	char c = get();

	switch (c) {
		case '>':
		{
			commit_font_state();

			auto newState = m_fontStates.back(); // copy last state
			newState.bold = true;
			newState.font = newState.family->get_font(FontFamily::WEIGHT_BOLD,
					newState.italic ? FontFaceStyle::ITALIC : newState.defaultStyle).get();

			m_fontStates.emplace_back(std::move(newState));
		}
			break;
		case 'r':
			parse_line_break();
			break;
		default:
			raise_error("b_tag: invalid char");
	}
}

void RichTextParser::parse_s_tag() {
	if (!has_next()) {
		raise_error("s_tag: does not have next");
		return;
	}

	char c = get();

	switch (c) {
		case '>':
			m_textInfo.strikethroughs.push_back({m_output.size(), 0});
			break;
		case 'c':
			if (!consume_char('>')) {
				return;
			}

			// FIXME: Smallcaps one day
			break;
		case 'm':
			if (!consume_word<8>("allcaps>")) {
				return;
			}

			// FIXME: Smallcaps one day
			break;
		case 't':
			parse_stroke();
			break;
		default:
			raise_error("s_tag: invalid char");
	}
}

void RichTextParser::parse_u_tag() {
	if (!has_next()) {
		raise_error("u_tag: does not have next");
		return;
	}

	char c = get();

	switch (c) {
		case '>':
			m_textInfo.underlines.push_back({m_output.size(), 0});
			break;
		case 'p':
			if (!consume_word<8>("percase>")) {
				return;
			}

			flag_uppercase_start();
			break;
		case 'c':
			if (!consume_char('>')) {
				return;
			}

			flag_uppercase_start();
			break;
		default:
			raise_error("u_tag: invalid char");
	}
}

void RichTextParser::parse_b_end_tag() {
	// only b with an end tag is </b>
	if (!consume_char('>')) {
		return;
	}

	commit_font_state();
	m_fontStates.pop_back();
}

void RichTextParser::parse_s_end_tag() {
	if (!has_next()) {
		raise_error("s_end_tag: does not have next");
		return;
	}

	char c = get();

	switch (c) {
		case '>':
			m_textInfo.strikethroughs.back().end = m_output.size();
			break;
		case 'c':
			if (!consume_char('>')) {
				return;
			}

			// FIXME: Smallcaps one day
			break;
		case 'm':
			if (!consume_word<8>("allcaps>")) {
				return;
			}

			// FIXME: Smallcaps one day
			break;
		case 't':
			parse_stroke_end();
			break;
		default:
			raise_error("s_tag: invalid char");
	}
}

void RichTextParser::parse_u_end_tag() {
	if (!has_next()) {
		raise_error("u_end_tag: does not have next");
		return;
	}

	char c = get();

	switch (c) {
		case '>':
			m_textInfo.underlines.back().end = m_output.size();
			break;
		case 'p':
			if (!consume_word<8>("percase>")) {
				return;
			}

			flag_uppercase_end();
			break;
		case 'c':
			if (!consume_char('>')) {
				return;
			}

			flag_uppercase_end();
			break;
		default:
			raise_error("u_end_tag: invalid char");
	}
}

void RichTextParser::parse_stroke() {
	if (!consume_word<4>("roke")) {
		return;
	}

	m_textInfo.strokeStates.emplace_back();
	auto& strokeState = m_textInfo.strokeStates.back();
	strokeState.begin = m_output.size();
	strokeState.value.thickness = 1;
	strokeState.value.joins = FontStrokeType::ROUND;

	parse_stroke_attributes();
}

void RichTextParser::parse_stroke_end() {
	if (!consume_word<5>("roke>")) {
		return;
	}

	m_textInfo.strokeStates.back().end = m_output.size();
}

void RichTextParser::parse_stroke_attributes() {
	for (;;) {
		if (!has_next()) {
			raise_error("stroke_attributes: does not have next");
			return;
		}

		char c = get();

		switch (c) {
			case 'c':
				parse_stroke_color();
				break;
			case 'j':
				parse_stroke_joins();
				break;
			case 't':
				parse_stroke_t_attributes();
				break;
			case ' ':
				break;
			case '>':
				return;
			default:
				raise_error("stroke_attributes: invalid char");
		}
	}
}

void RichTextParser::parse_stroke_color() {
	if (!consume_word<6>("olor=\"")) {
		return;
	}

	uint32_t color;

	if (!parse_color(color)) {
		return;
	}

	if (!consume_char('"')) {
		return;
	}

	m_textInfo.strokeStates.back().value.color = Math::Color3(color);
}

void RichTextParser::parse_stroke_joins() {
	if (!consume_word<6>("oins=\"")) {
		return;
	}

	const char* start = m_currPos;

	for (;;) {
		if (!has_next()) {
			raise_error("stroke_joins: does not have next");
			return;
		}

		char c = get();

		if (c == '"') {
			FontStrokeType joins = FontStrokeType::ROUND;

			std::string_view joinType(start, m_currPos - start - 1);

			if (joinType.compare("bevel") == 0) {
				joins = FontStrokeType::BEVEL;
			}
			else if (joinType.compare("miter") == 0) {
				joins = FontStrokeType::MITER;
			}

			m_textInfo.strokeStates.back().value.joins = joins;

			return;
		}
	}
}

void RichTextParser::parse_stroke_t_attributes() {
	if (!has_next()) {
		raise_error("stroke_t_attributes: does not have next");
		return;
	}

	char c = get();

	switch (c) {
		case 'h':
			parse_stroke_thickness();
			break;
		case 'r':
			parse_stroke_transparency();
			break;
		default:
			raise_error("stroke_t_attributes: invalid char");
	}
}

void RichTextParser::parse_stroke_thickness() {
	if (!consume_word<9>("ickness=\"")) {
		return;
	}

	uint32_t value{};

	if (!parse_numeric_with_stop(value, '"')) {
		return;
	}

	m_textInfo.strokeStates.back().value.thickness = static_cast<uint8_t>(value);
}

void RichTextParser::parse_stroke_transparency() {
	if (!consume_word<12>("ansparency=\"")) {
		return;
	}

	float value;

	if (!parse_numeric_with_stop(value, '"')) {
		return;
	}

	value = Math::min(Math::max(value, 0.f), 1.f);

	m_textInfo.strokeStates.back().value.transparency = value;
}

void RichTextParser::parse_line_break() {
	for (;;) {
		if (!has_next()) {
			raise_error("line_break: does not have next");
			return;
		}

		char c = get();

		switch (c) {
			case ' ':
				break;
			case '/':
				if (!consume_char('>')) {
					return;
				}

				m_output.push_back('\n');
				return;
			default:
				raise_error("line_break: invalid char");
				return;
		}
	}
}

bool RichTextParser::parse_color(uint32_t& value) {
	if (!has_next()) {
		raise_error("color: does not have next");
		return false;
	}

	char c = get();

	switch (c) {
		case '#':
			return parse_color_hex(value);
		case 'r':
			return parse_color_rgb(value);
		default:
			raise_error("color: invalid char");
			return false;
	}
}

bool RichTextParser::parse_color_hex(uint32_t& value) {
	if (!has_next_n(6)) {
		raise_error("font_color_hex: does not have next");
		return false;
	}

	if (auto res = std::from_chars(m_currPos, m_currPos + 6, value, 16);
			res.ptr != m_currPos + 6) {
		raise_error("font_color_hex: failed to parse color");
		return false;
	}

	advance_n(6);
	return true;
}

bool RichTextParser::parse_color_rgb(uint32_t& value) {
	if (!consume_word<3>("gb(")) {
		return false;
	}

	uint8_t r{};
	uint8_t g{};
	uint8_t b{};

	if (!parse_numeric_with_stop(r, ',')) {
		return false;
	}

	if (!parse_numeric_with_stop(g, ',')) {
		return false;
	}

	if (!parse_numeric_with_stop(b, ')')) {
		return false;
	}

	value = (r << 16) | (g << 8) | b;
	return true;
}

void RichTextParser::parse_font_color() {
	if (!consume_word<6>("olor=\"")) {
		return;
	}

	uint32_t color;

	if (!parse_color(color)) {
		return;
	}

	if (!consume_char('"')) {
		return;
	}

	auto& flags = m_fontModificationFlags.back();
	flags |= FLAG_HAS_COLOR_CHANGE;

	m_textInfo.colors.push_back({m_output.size(), 0, Math::Color3(color)});
}

void RichTextParser::parse_font_face() {
	if (!consume_word<5>("ace=\"")) {
		return;
	}

	const char* start = m_currPos;
	
	for (;;) {
		if (!has_next()) {
			raise_error("font_face: does not have next");
			return;
		}

		char c = get();

		if (c == '"') {
			Game::FontID fontID = Game::FontIDUtils::get_font_id_by_name(start,
					m_currPos - start - 1);

			if (fontID != Game::FontID::INVALID_FONT) {
				auto pFontFamily = Game::FontIDUtils::get_font_family_by_id(fontID);

				if (pFontFamily) {
					commit_font_state();

					auto& flags = m_fontModificationFlags.back();

					if (!(flags & FLAG_HAS_FACE_CHANGE)) {
						flags |= FLAG_HAS_FACE_CHANGE;
						m_fontStates.push_back(m_fontStates.back()); // Copy state
					}

					auto& newState = m_fontStates.back(); // Copy state
					newState.family = pFontFamily.get();
					Game::FontIDUtils::get_default_traits_by_id(fontID, newState.defaultWeight,
							newState.defaultStyle);

					if (newState.bold && newState.italic) {
						newState.font = pFontFamily->get_font(FontFamily::WEIGHT_BOLD,
								FontFaceStyle::ITALIC).get();
					}
					else if (newState.bold) {
						newState.font = pFontFamily->get_font(FontFamily::WEIGHT_BOLD,
								newState.defaultStyle).get();
					}
					else if (newState.italic) {
						newState.font = pFontFamily->get_font(newState.defaultWeight,
								FontFaceStyle::ITALIC).get();
					}
					else {
						newState.font = pFontFamily->get_font(newState.defaultWeight,
								newState.defaultStyle).get();
					}
				}
			}

			return;
		}
	}
}

void RichTextParser::parse_font_size() {
	if (!consume_word<5>("ize=\"")) {
		return;
	}

	uint32_t size = 0;

	if (!parse_numeric_with_stop(size, '"')) {
		return;
	}

	commit_font_state();

	auto& flags = m_fontModificationFlags.back();

	if (!(flags & FLAG_HAS_FACE_CHANGE)) {
		flags |= FLAG_HAS_FACE_CHANGE;
		m_fontStates.push_back(m_fontStates.back()); // Copy state
	}

	flags |= FLAG_HAS_SIZE_CHANGE;

	auto& newState = m_fontStates.back();
	newState.size = static_cast<uint8_t>(size + 1);

	m_textInfo.sizes.push_back({m_output.size(), 0, newState.size});
}

template <typename T>
bool RichTextParser::parse_numeric_with_stop(T& value, char stop) {
	const char* start = m_currPos;

	for (;;) {
		if (!has_next()) {
			raise_error("parse_numeric_with_stop: does not have next");
			return false;
		}

		char c = get();

		if (c == stop) {
			if constexpr (std::is_same_v<T, float>) {
				char* end;
				value = std::strtof(start, &end);
				return end == m_currPos - 1;
			}
			else {
				if (auto res = std::from_chars(start, m_currPos - 1, value);
						res.ptr != m_currPos - 1) {
					raise_error("parse_numeric_with_stop: malformed numeric");
					return false;
				}
			}
		}
	}
}

char RichTextParser::get() {
	return *(m_currPos++);
}

char RichTextParser::peek() const {
	return *m_currPos;
}

bool RichTextParser::has_next() const {
	return !m_error && m_currPos != m_end;
}

bool RichTextParser::has_next_n(size_t n) const {
	return !m_error && m_currPos + n < m_end;
}

void RichTextParser::advance() {
	++m_currPos;
}

void RichTextParser::advance_n(size_t n) {
	m_currPos += n;
}

bool RichTextParser::consume_char(char expected) {
	if (!has_next()) {
		return false;
	}

	char c = get();

	if (c != expected) {
		//char buffer[128] = {};
		//sprintf(buffer, "Expected char '%c' got '%c'", expected, c);
		//raise_error(buffer);
		raise_error("Unexpected char");
		return false;
	}

	return true;
}

template <size_t Size>
bool RichTextParser::consume_word(const char* word) {
	const char* end = word + Size;

	for (;;) {
		if (!consume_char(*word)) {
			return false;
		}

		++word;

		if (word == end) {
			return true;
		}
	}
}

void RichTextParser::raise_error(const char* msg) {
	(void)msg;
	m_error = true;
}

void RichTextParser::flag_uppercase_start() {
	commit_font_state();
	auto newState = m_fontStates.back();
	newState.modifier = Font::Modifier::UPPERCASE;
	m_fontStates.emplace_back(std::move(newState));
}

void RichTextParser::flag_uppercase_end() {
	commit_font_state();
	m_fontStates.pop_back();
}

void RichTextParser::commit_font_state() {
	auto& state = m_fontStates.back();

	if (m_textIndex >= m_output.size()) {
		return;
	}

	state.font->get_glyph_positions_with_fallback(&m_output[m_textIndex],
			m_output.size() - m_textIndex, state.size, state.modifier, m_glyphPositions,
			m_textCursor, static_cast<uint32_t>(m_textIndex));

	m_textIndex = m_output.size();
}

