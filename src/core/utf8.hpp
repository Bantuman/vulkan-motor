#pragma once

#include <cstdint>

namespace UTF8 {
	uint32_t read_code_point(const char* str);
	uint32_t write_code_point(uint32_t codepoint, char* buffer);

	class CodePointStream {
		public:
			explicit CodePointStream(const char* strStart, const char* strEnd);

			uint32_t peek() const;
			uint32_t get();

			const char* get_position() const;

			bool good() const;
			operator bool() const;
		private:
			const char* m_position;
			const char* m_end;
	};
}

