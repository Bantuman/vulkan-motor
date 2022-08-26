#pragma once

#include <core/common.hpp>

#include <math/vector2.hpp>
#include <math/vector4.hpp>

#include <memory>
#include <rendering/image.hpp>
#include <rendering/image_view.hpp>
#include <rendering/font_stroke_type.hpp>

#include <unordered_map>

class Font;

class TextBitmap {
	public:
		explicit TextBitmap();

		NULL_COPY_AND_ASSIGN(TextBitmap);

		void get_char_info(Font&, uint32_t glyphIndex, uint8_t size, Math::Vector4& texExtentsOut,
				Math::Vector2& sizeOut, Math::Vector2& offsetOut, bool& drawableOut);

		void get_stroke_info(Font&, uint32_t glyphIndex, uint8_t size, FontStrokeType,
				uint8_t strokeSize, Math::Vector4& texExtentsOut, Math::Vector2& sizeOut);

		VkImage get_bitmap_image() const;
		VkImageView get_bitmap_image_view() const;
	private:
		struct InfoKey {
			const Font* font;
			uint32_t glyphIndex;
			uint8_t size;

			bool operator==(const InfoKey&) const;
			size_t hash() const;
		};

		struct StrokeKey {
			const Font* font;
			uint32_t glyphIndex;
			uint8_t size;
			FontStrokeType strokeType;
			uint8_t strokeSize;

			bool operator==(const StrokeKey&) const;
			size_t hash() const;
		};

		struct CharInfo {
			Math::Vector4 texExtents;
			Math::Vector2 bitmapSize;
			Math::Vector2 offset;
			bool drawable;
		};

		struct StrokeInfo {
			Math::Vector4 texExtents;
			Math::Vector2 bitmapSize;
		};

		struct InfoKeyHash {
			size_t operator()(const InfoKey& k) const {
				return k.hash();
			}
		};

		struct StrokeKeyHash {
			size_t operator()(const StrokeKey& k) const {
				return k.hash();
			}
		};

		std::shared_ptr<Image> m_bitmap;
		std::shared_ptr<ImageView> m_bitmapView;

		uint32_t m_xOffset;
		uint32_t m_yOffset;
		uint32_t m_currLineHeight;

		std::unordered_map<InfoKey, CharInfo, InfoKeyHash> m_extentsMap;
		std::unordered_map<StrokeKey, StrokeInfo, StrokeKeyHash> m_strokeMap;
};

