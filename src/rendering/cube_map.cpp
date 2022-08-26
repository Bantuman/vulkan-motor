#include "cube_map.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <file/file_system.hpp>
#include <path_utils.hpp>
#include <rendering/dds_texture.hpp>
#include <rendering/vk_initializers.hpp>

#include <stb_image.h>

static std::shared_ptr<CubeMap> load_from_multiple_images(RenderContext& ctx,
		const std::string_view* fileNames, bool srgb);
static std::shared_ptr<CubeMap> load_from_multiple_images_dds(RenderContext& ctx,
		const std::string_view* fileNames, bool srgb);
static std::shared_ptr<CubeMap> load_from_multiple_images_stb(RenderContext& ctx,
		const std::string_view* fileNames, bool srgb);

CubeMap::CubeMap(const std::shared_ptr<Image> image, std::shared_ptr<ImageView> imageView)
		: m_image(std::move(image))
		, m_imageView(std::move(imageView)) {}

std::shared_ptr<Image> CubeMap::get_image() const {
	return m_image;
}

std::shared_ptr<ImageView> CubeMap::get_image_view() const {
	return m_imageView;
}

std::shared_ptr<CubeMap> CubeMapLoader::load(RenderContext& ctx, const std::string_view* fileNames,
		size_t numFileNames, bool srgb) {
	assert(fileNames);
	assert(numFileNames == 1 || numFileNames == 6);

	if (numFileNames == 6) {
		return load_from_multiple_images(ctx, fileNames, srgb);
	}
	else {
		return nullptr;
	}
}


static std::shared_ptr<CubeMap> load_from_multiple_images(RenderContext& ctx,
		const std::string_view* fileNames, bool srgb) {
	auto ext = PathUtils::get_file_extension(fileNames[0]);

	if (ext.compare("dds") == 0) {
		return load_from_multiple_images_dds(ctx, fileNames, srgb);
	}
	else {
		return load_from_multiple_images_stb(ctx, fileNames, srgb);
	}

	auto data = g_fileSystem->file_read_bytes(fileNames[0]);

	if (data.empty()) {
		return {};
	}

	int firstWidth, firstHeight, firstChannels;
	stbi_uc* firstImageData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data.data()),
			data.size(), &firstWidth, &firstHeight, &firstChannels, STBI_rgb_alpha);

	if (!firstImageData) {
		return {};
	}

	size_t singleImageSize = firstWidth * firstHeight * 4;
	size_t totalSize = singleImageSize * 6;
	uint8_t* buffer = reinterpret_cast<uint8_t*>(malloc(totalSize));

	VkExtent3D extents = {
		static_cast<uint32_t>(firstWidth),
		static_cast<uint32_t>(firstHeight),
		1,
	};

	VkFormat format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	int usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	memcpy(buffer, firstImageData, singleImageSize);
	stbi_image_free(firstImageData);

	for (size_t i = 1; i < 6; ++i) {
		auto data = g_fileSystem->file_read_bytes(fileNames[i]);

		if (data.empty()) {
			free(buffer);
			return {};
		}

		int width, height, channels;
		stbi_uc* imageData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data.data()),
				data.size(), &width, &height, &channels, STBI_rgb_alpha);

		if (!imageData) {
			free(buffer);
			return {};
		}

		if (width != firstWidth || height != firstHeight) {
			stbi_image_free(imageData);
			free(buffer);
			return {};
		}

		memcpy(buffer + (singleImageSize * i), imageData, singleImageSize);
		stbi_image_free(imageData);
	}

	auto createInfo = vkinit::image_create_info(format, usageFlags, extents);
	createInfo.arrayLayers = 6;
	createInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	auto image = ctx.image_create(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);

	auto imageView = ctx.image_view_create(image, VK_IMAGE_VIEW_TYPE_CUBE, format,
			VK_IMAGE_ASPECT_COLOR_BIT, 1, 6);

	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 6;

	ctx.staging_context_create()
			->add_image(*image, buffer, totalSize, range, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_ACCESS_SHADER_READ_BIT, [buffer] { free(buffer); })
			.submit();

	return std::make_shared<CubeMap>(image, imageView);
}

static std::shared_ptr<CubeMap> load_from_multiple_images_dds(RenderContext& ctx,
		const std::string_view* fileNames, bool srgb) {
	auto data = g_fileSystem->file_read_bytes(fileNames[0]);

	if (data.empty()) {
		return {};
	}

	uint32_t firstWidth;
	uint32_t firstHeight;
	uint32_t firstSize;
	uint32_t totalSize;
	VkFormat format;
	uint8_t* buffer = nullptr;

	{
		DDS::DDSTextureInfo texInfo{};

		if (!DDS::get_texture_info(texInfo, data.data(), data.size(), srgb)) {
			return {};
		}

		firstWidth = texInfo.width;
		firstHeight = texInfo.height;
		firstSize = texInfo.size;
		format = texInfo.format;

		totalSize = 6 * texInfo.size;

		buffer = reinterpret_cast<uint8_t*>(malloc(totalSize));
		memcpy(buffer, texInfo.dataStart, texInfo.size);
	}

	VkExtent3D extents = {
		firstWidth,
		firstHeight,
		1,
	};

	int usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	for (size_t i = 1; i < 6; ++i) {
		auto data = g_fileSystem->file_read_bytes(fileNames[i]);

		if (data.empty()) {
			free(buffer);
			return {};
		}

		DDS::DDSTextureInfo texInfo{};

		if (!DDS::get_texture_info(texInfo, data.data(), data.size(), srgb)) {
			free(buffer);
			return {};
		}

		if (texInfo.width != firstWidth || texInfo.height != firstHeight
				|| texInfo.size != firstSize || texInfo.format != format) {
			free(buffer);
			return {};
		}

		memcpy(buffer + (firstSize * i), texInfo.dataStart, firstSize);
	}

	auto createInfo = vkinit::image_create_info(format, usageFlags, extents);
	createInfo.arrayLayers = 6;
	createInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	auto image = ctx.image_create(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);

	auto imageView = ctx.image_view_create(image, VK_IMAGE_VIEW_TYPE_CUBE, format,
			VK_IMAGE_ASPECT_COLOR_BIT, 1, 6);

	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 6;

	ctx.staging_context_create()
			->add_image(*image, buffer, totalSize, range, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_ACCESS_SHADER_READ_BIT, [buffer] { free(buffer); })
			.submit();

	return std::make_shared<CubeMap>(image, imageView);
}

static std::shared_ptr<CubeMap> load_from_multiple_images_stb(RenderContext& ctx,
		const std::string_view* fileNames, bool srgb) {
	auto data = g_fileSystem->file_read_bytes(fileNames[0]);

	if (data.empty()) {
		return {};
	}

	int firstWidth, firstHeight, firstChannels;
	stbi_uc* firstImageData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data.data()),
			data.size(), &firstWidth, &firstHeight, &firstChannels, STBI_rgb_alpha);

	if (!firstImageData) {
		return {};
	}

	size_t singleImageSize = firstWidth * firstHeight * 4;
	size_t totalSize = singleImageSize * 6;
	uint8_t* buffer = reinterpret_cast<uint8_t*>(malloc(totalSize));

	VkExtent3D extents = {
		static_cast<uint32_t>(firstWidth),
		static_cast<uint32_t>(firstHeight),
		1,
	};

	VkFormat format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	int usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	memcpy(buffer, firstImageData, singleImageSize);
	stbi_image_free(firstImageData);

	for (size_t i = 1; i < 6; ++i) {
		auto data = g_fileSystem->file_read_bytes(fileNames[i]);

		if (data.empty()) {
			free(buffer);
			return {};
		}

		int width, height, channels;
		stbi_uc* imageData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data.data()),
				data.size(), &width, &height, &channels, STBI_rgb_alpha);

		if (!imageData) {
			free(buffer);
			return {};
		}

		if (width != firstWidth || height != firstHeight) {
			stbi_image_free(imageData);
			free(buffer);
			return {};
		}

		memcpy(buffer + (singleImageSize * i), imageData, singleImageSize);
		stbi_image_free(imageData);
	}

	auto createInfo = vkinit::image_create_info(format, usageFlags, extents);
	createInfo.arrayLayers = 6;
	createInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	auto image = ctx.image_create(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);

	auto imageView = ctx.image_view_create(image, VK_IMAGE_VIEW_TYPE_CUBE, format,
			VK_IMAGE_ASPECT_COLOR_BIT, 1, 6);

	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 6;

	ctx.staging_context_create()
			->add_image(*image, buffer, totalSize, range, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_ACCESS_SHADER_READ_BIT, [buffer] { free(buffer); })
			.submit();

	return std::make_shared<CubeMap>(image, imageView);
}

