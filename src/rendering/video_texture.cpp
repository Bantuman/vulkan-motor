#include "video_texture.hpp"

#include <file/file_system.hpp>
#include <path_utils.hpp>
#include <rendering/vk_initializers.hpp>

#include <cmath>

// FIXME: Implement

static std::shared_ptr<VideoTexture> load_from_mp4(RenderContext&, const std::vector<char>&, bool,
	bool);
static std::shared_ptr<VideoTexture> load_from_webm(RenderContext&, const std::vector<char>&, bool,
	bool);

VideoTexture::VideoTexture(std::shared_ptr<Image> image, std::shared_ptr<ImageView> imageView,
	uint32_t numMipMaps)
	: m_image(std::move(image))
	, m_imageView(std::move(imageView))
	, m_numMipMaps(numMipMaps) {}

std::shared_ptr<Image> VideoTexture::get_image() const {
	return m_image;
}

std::shared_ptr<ImageView> VideoTexture::get_image_view() const {
	return m_imageView;
}

uint32_t VideoTexture::get_num_mip_maps() const {
	return m_numMipMaps;
}

float VideoTexture::get_milliseconds() const
{
	return 0.0f;
}

void VideoTexture::set_milliseconds(float)
{
}

uint32_t VideoTexture::get_frame() const
{
	return uint32_t();
}

void VideoTexture::set_frame(uint32_t)
{
}

std::shared_ptr<VideoTexture> VideoTextureLoader::load(RenderContext& ctx, const std::string_view& fileName, bool srgb, bool generateMipmaps) {
	std::vector<char> data = g_fileSystem->file_read_bytes(fileName);

	if (data.empty()) {
		return {};
	}

	auto ext = PathUtils::get_file_extension(fileName);

	if (ext.compare("mp4") == 0) {
		return load_from_mp4(ctx, data, srgb, generateMipmaps);
	}
	else {
		return load_from_webm(ctx, data, srgb, generateMipmaps);
	}
}

static std::shared_ptr<VideoTexture> load_from_webm(RenderContext& /*ctx*/, const std::vector<char>& /*data*/, bool /*srgb*/, bool /*generateMipmaps*/) {
	return std::shared_ptr<VideoTexture>();
}

static std::shared_ptr<VideoTexture> load_from_mp4(RenderContext& /*ctx*/, const std::vector<char>& /*data*/, bool /*srgb*/, bool /*generateMipmaps*/) {
	return std::shared_ptr<VideoTexture>();
}

