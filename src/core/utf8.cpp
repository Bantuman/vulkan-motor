#include "utf8.hpp"

uint32_t UTF8::read_code_point(const char* str) {
	if ((*str & 0b1111'0000) == 0b1111'0000) {
		return ((str[0] & 0b0000'0111) << 18)
				| ((str[1] & 0b0011'1111) << 12)
				| ((str[2] & 0b0011'1111) << 6)
				| (str[3] & 0b0011'1111);
	}
	else if ((*str & 0b1110'0000) == 0b1110'0000) {
		return ((str[0] & 0b0000'1111) << 12)
				| ((str[1] & 0b0011'1111) << 6)
				| (str[2] & 0b0011'1111);
	}
	else if ((*str & 0b1100'0000) == 0b1100'0000) {
		return ((str[0] & 0b0001'1111) << 6)
				| (str[1] & 0b0011'1111);
	}
	else {
		return static_cast<uint32_t>(*str);
	}
}

uint32_t UTF8::write_code_point(uint32_t codePoint, char* buffer) {
	if (codePoint <= 0x7F) {
		buffer[0] = static_cast<char>(codePoint);

		return 1;
	}
	else if (codePoint <= 0x7FF) {
		buffer[0] = static_cast<char>(0b1100'0000 | ((codePoint >> 6) & 0b0001'1111));
		buffer[1] = static_cast<char>(0b1000'0000 | (codePoint & 0b0011'1111));

		return 2;
	}
	else if (codePoint <= 0xFFFF) {
		buffer[0] = static_cast<char>(0b1110'0000 | ((codePoint >> 12) & 0b0000'1111));
		buffer[1] = static_cast<char>(0b1000'0000 | ((codePoint >> 6) & 0b0011'1111));
		buffer[2] = static_cast<char>(0b1000'0000 | (codePoint & 0b0011'1111));

		return 3;
	}
	else {
		buffer[0] = static_cast<char>(0b1111'0000 | ((codePoint >> 18) & 0b0000'0111));
		buffer[1] = static_cast<char>(0b1000'0000 | ((codePoint >> 12) & 0b0011'1111));
		buffer[2] = static_cast<char>(0b1000'0000 | ((codePoint >> 6) & 0b0011'1111));
		buffer[3] = static_cast<char>(0b1000'0000 | (codePoint & 0b0011'1111));

		return 4;
	}
}

// CodePointStream

UTF8::CodePointStream::CodePointStream(const char* strStart, const char* strEnd)
		: m_position(strStart)
		, m_end(strEnd) {}

uint32_t UTF8::CodePointStream::peek() const {
	return UTF8::read_code_point(m_position);
}

uint32_t UTF8::CodePointStream::get() {
	uint32_t res;

	if ((*m_position & 0b1111'0000) == 0b1111'0000) {
		res = ((m_position[0] & 0b0000'0111) << 18)
				| ((m_position[1] & 0b0011'1111) << 12)
				| ((m_position[2] & 0b0011'1111) << 6)
				| (m_position[3] & 0b0011'1111);
		m_position += 4;
	}
	else if ((*m_position & 0b1110'0000) == 0b1110'0000) {
		res = ((m_position[0] & 0b0000'1111) << 12)
				| ((m_position[1] & 0b0011'1111) << 6)
				| (m_position[2] & 0b0011'1111);
		m_position += 3;
	}
	else if ((*m_position & 0b1100'0000) == 0b1100'0000) {
		res = ((m_position[0] & 0b0001'1111) << 6)
				| (m_position[1] & 0b0011'1111);
		m_position += 2;
	}
	else {
		res = static_cast<uint32_t>(*m_position);
		++m_position;
	}

	return res;
}

const char* UTF8::CodePointStream::get_position() const {
	return m_position;
}

bool UTF8::CodePointStream::good() const {
	return m_position != m_end;
}

UTF8::CodePointStream::operator bool() const {
	return good();
}

