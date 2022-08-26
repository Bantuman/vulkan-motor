#include "text_bitmap.hpp"

#include <math/common.hpp>

#include <rendering/render_context.hpp>
#include <rendering/vk_initializers.hpp>
#include <rendering/freetype.hpp>
#include <rendering/font.hpp>

#include FT_STROKER_H

static constexpr uint32_t TEXTURE_EXTENT = 2048u;
static constexpr uint32_t TEXTURE_PADDING = 1u;

static void upload_image(VkCommandBuffer cmd, VkBuffer buffer, VkImage image, int32_t xOffset,
		int32_t yOffset, uint32_t uWidth, uint32_t uHeight);

// InfoKey

bool TextBitmap::InfoKey::operator==(const InfoKey& other) const {
	return font == other.font && glyphIndex == other.glyphIndex && size == other.size;
}

size_t TextBitmap::InfoKey::hash() const {
	return reinterpret_cast<uint64_t>(font)
			^ (static_cast<uint64_t>(size) << 32)
			^ static_cast<uint64_t>(glyphIndex);
}

// StrokeKey

bool TextBitmap::StrokeKey::operator==(const StrokeKey& other) const {
	return font == other.font && glyphIndex == other.glyphIndex && size == other.size
			&& strokeType == other.strokeType && strokeSize == other.strokeSize;
}

size_t TextBitmap::StrokeKey::hash() const {
	return reinterpret_cast<uint64_t>(font)
			^ (static_cast<uint64_t>(size) << 32)
			^ static_cast<uint64_t>(glyphIndex)
			^ (static_cast<uint64_t>(strokeType) << 40)
			^ (static_cast<uint64_t>(strokeSize) << 48);
}

bool glyphBit(const FT_GlyphSlot& glyph, const int x, const int y)
{
	int pitch = abs(glyph->bitmap.pitch);
	unsigned char* row = &glyph->bitmap.buffer[pitch * y];
	char cValue = row[x >> 3];

	return (cValue & (128 >> (x & 7))) != 0;
}
// TextBitmap

TextBitmap::TextBitmap()
		: m_xOffset(0)
		, m_yOffset(0)
		, m_currLineHeight(0) {
	VkExtent3D extents = {
		TEXTURE_EXTENT,
		TEXTURE_EXTENT,
		1
	};

	auto imageInfo = vkinit::image_create_info(VK_FORMAT_R8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, extents);

	m_bitmap = g_renderContext->image_create(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY);

	auto viewInfo = vkinit::image_view_create_info(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8_UNORM,
			*m_bitmap, VK_IMAGE_ASPECT_COLOR_BIT);
	viewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_R};
	
	m_bitmapView = g_renderContext->image_view_create(viewInfo);

	g_renderContext->immediate_submit([&](VkCommandBuffer cmd) {
		VkImageMemoryBarrier toGraphics{};
		toGraphics.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		toGraphics.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		toGraphics.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		toGraphics.srcAccessMask = 0;
		toGraphics.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		toGraphics.image = *m_bitmap;
		toGraphics.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		toGraphics.subresourceRange.levelCount = 1;
		toGraphics.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toGraphics);
	});
}

void TextBitmap::get_char_info(Font& font, uint32_t glyphIndex, uint8_t size,
		Math::Vector4& texExtentsOut, Math::Vector2& sizeOut, Math::Vector2& offsetOut,
		bool& drawableOut) {
	InfoKey key{&font, glyphIndex, size};

	if (auto it = m_extentsMap.find(key); it != m_extentsMap.end()) {
		texExtentsOut = it->second.texExtents;
		sizeOut = it->second.bitmapSize;
		offsetOut = it->second.offset;
		drawableOut = it->second.drawable;
		return;
	}

	FT_Face face = &font.get_font_info();

	font.set_pixel_size(size);

	FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER | (!font.get_antialiased() * FT_LOAD_MONOCHROME));

	auto uWidth = static_cast<uint32_t>(face->glyph->bitmap.width);
	auto uHeight = static_cast<uint32_t>(face->glyph->bitmap.rows);
	auto uPitch = static_cast<uint32_t>(face->glyph->bitmap.pitch);

	drawableOut = uWidth > 0 && uHeight > 0;

	if (!drawableOut) {
		CharInfo info{};
		info.drawable = false;
		m_extentsMap.emplace(std::make_pair(key, std::move(info)));
		return;
	}

	auto tmpBuf = g_renderContext->buffer_create(uWidth * 8 * uHeight * 8,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	uint8_t* tmpMapping = reinterpret_cast<uint8_t*>(tmpBuf->map());

	
	if (font.get_antialiased())
	{
		for (uint32_t i = 0; i < uWidth * uHeight; ++i)
		{
			uint8_t v = face->glyph->bitmap.buffer[i];
			float f = Math::pow(static_cast<float>(v) / 255.f, 1.f / 2.2f);
			tmpMapping[i] = static_cast<uint8_t>(f * 255.f);
		}
	}
	else
	{
		for (uint32_t y = 0; y < uHeight; ++y) 
		{
			for (uint32_t i = 0; i < uPitch; ++i) 
			{
				auto bv = face->glyph->bitmap.buffer[y * uPitch + i];
				auto row = y * uWidth + i * 8;
				for (uint32_t k = 0; k < std::min(8u, uWidth - i * 8); ++k)
				{
					tmpMapping[row + k] = ((bv & (1 << (7 - k))) != 0) * 255;
				}
			}
		}
	
	}
	
	uint32_t padWidth = uWidth + TEXTURE_PADDING;
	uint32_t padHeight = uHeight + TEXTURE_PADDING;

	if (m_xOffset + padWidth > TEXTURE_EXTENT) {
		m_xOffset = 0;
		m_yOffset += m_currLineHeight;
		m_currLineHeight = padHeight;
	}

	g_renderContext->immediate_submit([&](VkCommandBuffer cmd) {
		upload_image(cmd, *tmpBuf, *m_bitmap, static_cast<int32_t>(m_xOffset),
				static_cast<int32_t>(m_yOffset), uWidth, uHeight);
	});

	texExtentsOut = Math::Vector4(static_cast<float>(m_xOffset), static_cast<float>(m_yOffset),
			static_cast<float>(uWidth), static_cast<float>(uHeight))
			/ static_cast<float>(TEXTURE_EXTENT);

	m_xOffset += padWidth;

	if (m_currLineHeight < padHeight) {
		m_currLineHeight = padHeight;
	}

	sizeOut = Math::Vector2(static_cast<float>(uWidth), static_cast<float>(uHeight));
	offsetOut = Math::Vector2(static_cast<float>(face->glyph->bitmap_left),
			static_cast<float>(-face->glyph->bitmap_top));
	m_extentsMap.emplace(std::make_pair(key,
				CharInfo{texExtentsOut, sizeOut, offsetOut, true}));
}

void TextBitmap::get_stroke_info(Font& font, uint32_t glyphIndex, uint8_t size,
		FontStrokeType strokeType, uint8_t strokeSize, Math::Vector4& texExtentsOut,
		Math::Vector2& sizeOut) {
	StrokeKey key{&font, glyphIndex, size, strokeType, strokeSize};

	if (auto it = m_strokeMap.find(key); it != m_strokeMap.end()) {
		texExtentsOut = it->second.texExtents;
		sizeOut = it->second.bitmapSize;
		return;
	}

	FT_Face face = &font.get_font_info();

	font.set_pixel_size(size);

	//FT_Load_Char(face, c, FT_LOAD_NO_BITMAP);
	FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP);

	FT_Glyph glyph;
	FT_Get_Glyph(face->glyph, &glyph);

	glyph->format = FT_GLYPH_FORMAT_OUTLINE;

	FT_Stroker_LineCap lineCap = FT_STROKER_LINECAP_ROUND;
	FT_Stroker_LineJoin lineJoin = FT_STROKER_LINEJOIN_ROUND;

	switch (strokeType) {
		case FontStrokeType::BEVEL:
			lineJoin = FT_STROKER_LINEJOIN_BEVEL;
			break;
		case FontStrokeType::MITER:
			lineJoin = FT_STROKER_LINEJOIN_MITER;
			break;
		default:
			break;
	}

	FT_Stroker stroker;
	FT_Stroker_New(g_freeType, &stroker);
	FT_Stroker_Set(stroker, static_cast<FT_Long>(strokeSize) * 64, lineCap, lineJoin, 0);

	FT_Glyph_Stroke(&glyph, stroker, false);
	FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, true);
	auto bmpGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);

	auto uWidth = static_cast<uint32_t>(bmpGlyph->bitmap.width);
	auto uHeight = static_cast<uint32_t>(bmpGlyph->bitmap.rows);

	auto tmpBuf = g_renderContext->buffer_create(uWidth * uHeight,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	uint8_t* tmpMapping = reinterpret_cast<uint8_t*>(tmpBuf->map());

	for (uint32_t i = 0; i < uWidth * uHeight; ++i) {
		uint8_t v = bmpGlyph->bitmap.buffer[i];
		float f = Math::pow(static_cast<float>(v) / 255.f, 1.f / 2.2f);
		tmpMapping[i] = static_cast<uint8_t>(f * 255.f);
	}

	uint32_t padWidth = uWidth + TEXTURE_PADDING;
	uint32_t padHeight = uHeight + TEXTURE_PADDING;

	if (m_xOffset + padWidth > TEXTURE_EXTENT) {
		m_xOffset = 0;
		m_yOffset += m_currLineHeight;
		m_currLineHeight = padHeight;
	}

	g_renderContext->immediate_submit([&](VkCommandBuffer cmd) {
		upload_image(cmd, *tmpBuf, *m_bitmap, static_cast<int32_t>(m_xOffset),
				static_cast<int32_t>(m_yOffset), uWidth, uHeight);
	});

	texExtentsOut = Math::Vector4(static_cast<float>(m_xOffset), static_cast<float>(m_yOffset),
			static_cast<float>(uWidth), static_cast<float>(uHeight))
			/ static_cast<float>(TEXTURE_EXTENT);

	m_xOffset += padWidth;

	if (m_currLineHeight < padHeight) {
		m_currLineHeight = padHeight;
	}

	sizeOut = Math::Vector2(static_cast<float>(uWidth), static_cast<float>(uHeight));
	m_strokeMap.emplace(std::make_pair(key, StrokeInfo{texExtentsOut, sizeOut}));

	FT_Stroker_Done(stroker);
	FT_Done_Glyph(glyph);
}

VkImage TextBitmap::get_bitmap_image() const {
	return *m_bitmap;
}

VkImageView TextBitmap::get_bitmap_image_view() const {
	return *m_bitmapView;
}

// Utils

static void upload_image(VkCommandBuffer cmd, VkBuffer buffer, VkImage image, int32_t xOffset,
		int32_t yOffset, uint32_t uWidth, uint32_t uHeight) {
	VkImageMemoryBarrier toTransfer{};
	toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	toTransfer.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	toTransfer.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	toTransfer.image = image;
	toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	toTransfer.subresourceRange.levelCount = 1;
	toTransfer.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransfer);

	VkBufferImageCopy copy{};
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.layerCount = 1;
	copy.imageOffset = { xOffset, yOffset, 0 };
	copy.imageExtent = { uWidth, uHeight, 1 };

	vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

	VkImageMemoryBarrier toGraphics = toTransfer;
	toGraphics.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	toGraphics.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	toGraphics.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	toGraphics.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toGraphics);
}

