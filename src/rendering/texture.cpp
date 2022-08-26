#include "texture.hpp"

#include <file/file_system.hpp>
#include <path_utils.hpp>
#include <rendering/dds_texture.hpp>
#include <rendering/vk_initializers.hpp>

#include <stb_image.h>

#include <cmath>

static std::shared_ptr<Texture> load_from_dds(RenderContext&, const std::vector<char>&, bool,
		bool);
static std::shared_ptr<Texture> load_from_stb(RenderContext&, const std::vector<char>&, bool,
		bool);

Texture::Texture(std::shared_ptr<Image> image, std::shared_ptr<ImageView> imageView,
			uint32_t numMipMaps)
		: m_image(std::move(image))
		, m_imageView(std::move(imageView))
		, m_numMipMaps(numMipMaps) {}

std::shared_ptr<Image> Texture::get_image() const {
	return m_image;
}

std::shared_ptr<ImageView> Texture::get_image_view() const {
	return m_imageView;
}

uint32_t Texture::get_num_mip_maps() const {
	return m_numMipMaps;
}

std::shared_ptr<Texture> TextureLoader::load(RenderContext& ctx, const std::string_view& fileName, bool srgb, bool generateMipmaps) {
	std::vector<char> data = g_fileSystem->file_read_bytes(fileName);

	if (data.empty()) {
		return {};
	}

	auto ext = PathUtils::get_file_extension(fileName);

	if (ext.compare("dds") == 0) {
		return load_from_dds(ctx, data, srgb, generateMipmaps);
	}
	else {
		return load_from_stb(ctx, data, srgb, generateMipmaps);
	}
}

static std::shared_ptr<Texture> load_from_dds(RenderContext& ctx, const std::vector<char>& data, bool srgb, bool generateMipmaps) {
	DDS::DDSTextureInfo textureInfo{};

	if (!DDS::get_texture_info(textureInfo, data.data(), data.size(), srgb)) {
		return {};
	}

	VkExtent3D extents = {
		static_cast<uint32_t>(textureInfo.width),
		static_cast<uint32_t>(textureInfo.height),
		1
	};

	uint32_t mipLevels = generateMipmaps
		? static_cast<uint32_t>(floor(log2(std::max(textureInfo.width, textureInfo.height)))) + 1 : 1;

	int usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	auto createInfo = vkinit::image_create_info(textureInfo.format, usageFlags,
			extents);
	createInfo.mipLevels = mipLevels;

	auto image = ctx.image_create(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);

	if (!image) {
		return {};
	}

	auto imageView = ctx.image_view_create(image, VK_IMAGE_VIEW_TYPE_2D, textureInfo.format,
			VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	if (!imageView) {
		return {};
	}

	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = mipLevels;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	ctx.staging_context_create()
		->add_image(*image, textureInfo.dataStart, textureInfo.size,
				range, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
				[data] {})
		.submit();

	return std::make_shared<Texture>(image, imageView, mipLevels);
}

static std::shared_ptr<Texture> load_from_stb(RenderContext& ctx, const std::vector<char>& data,
		bool srgb, bool generateMipmaps) {
	int width, height, channels;
	stbi_uc* imageData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data.data()),
			data.size(), &width, &height, &channels, STBI_rgb_alpha);

	if (!imageData) {
		return {};
	}

	VkExtent3D extents = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		1
	};

	VkFormat format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

	uint32_t mipLevels = generateMipmaps
		? static_cast<uint32_t>(floor(log2(std::max(width, height)))) + 1 : 1;

	int usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if (generateMipmaps) {
		usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	auto createInfo = vkinit::image_create_info(format, usageFlags, extents);
	createInfo.mipLevels = mipLevels;

	auto image = ctx.image_create(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);

	if (!image) {
		stbi_image_free(imageData);
		return {};
	}

	auto imageView = ctx.image_view_create(image, VK_IMAGE_VIEW_TYPE_2D, format,
			VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	if (!imageView) {
		stbi_image_free(imageData);
		return {};
	}

	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = mipLevels;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	ctx.staging_context_create()
		->add_image(*image, imageData, static_cast<size_t>(width * height * 4),
				range, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
				[imageData] { stbi_image_free(imageData); })
		.submit();

	return std::make_shared<Texture>(image, imageView, mipLevels);
}

