#pragma once

#include <core/common.hpp>

#include <string>
#include <memory>
#include <vector>

enum class FontFaceStyle : uint8_t {
	NORMAL = 0,
	ITALIC = 1,
	NOAA = 2,
	
	NUM_STYLES
};

struct FontFaceTraits {
	std::string path;
	uint32_t weight;
};

class Font;

class FontFamily {
	public:
		static constexpr const uint32_t WEIGHT_EXTRA_LIGHT = 200;
		static constexpr const uint32_t WEIGHT_LIGHT = 300;
		static constexpr const uint32_t WEIGHT_DEFAULT = 400;
		static constexpr const uint32_t WEIGHT_MEDIUM = 500;
		static constexpr const uint32_t WEIGHT_SEMI_BOLD = 600;
		static constexpr const uint32_t WEIGHT_BOLD = 700;
		static constexpr const uint32_t WEIGHT_EXTRA_BOLD = 800;
		static constexpr const uint32_t WEIGHT_BLACK = 900;

		explicit FontFamily(std::vector<FontFaceTraits>&& normalTraits,
				std::vector<FontFaceTraits>&& italicTraits);

		NULL_COPY_AND_ASSIGN(FontFamily);

		std::shared_ptr<Font> get_font(uint32_t weight, FontFaceStyle) const;
		std::shared_ptr<Font> get_default_font() const;

		const std::string& get_font_path(uint32_t weight, FontFaceStyle) const;
		const std::string& get_default_font_path() const;
	private:
		std::vector<FontFaceTraits> m_faceTraits[static_cast<uint8_t>(FontFaceStyle::NUM_STYLES)];
};

struct FontFamilyLoader {
	std::shared_ptr<FontFamily> load(const std::string_view& fileName);
};

