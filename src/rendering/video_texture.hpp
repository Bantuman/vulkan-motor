#pragma once

#include <rendering/render_context.hpp>

extern "C"
{
#include "ffmpeg-2.0/libavcodec/avcodec.h"
#include "ffmpeg-2.0/libavdevice/avdevice.h"
#include "ffmpeg-2.0/libavfilter/avfilter.h"
#include "ffmpeg-2.0/libavformat/avformat.h"
#include "ffmpeg-2.0/libavformat/avio.h"
#include "ffmpeg-2.0/libavutil/avutil.h"
#include "ffmpeg-2.0/libpostproc/postprocess.h"
#include "ffmpeg-2.0/libswresample/swresample.h"
#include "ffmpeg-2.0/libswscale/swscale.h"
}

class VideoTexture {
public:
	explicit VideoTexture(std::shared_ptr<Image> video, std::shared_ptr<ImageView> imageView,
		uint32_t numMipMaps);

	std::shared_ptr<Image> get_image() const;
	std::shared_ptr<ImageView> get_image_view() const;
	uint32_t get_num_mip_maps() const;
	float get_milliseconds() const;
	void set_milliseconds(float milliseconds);
	uint32_t get_frame() const;
	void set_frame(uint32_t);

private:
	std::shared_ptr<Image> m_image;
	std::shared_ptr<ImageView> m_imageView;
	uint32_t m_numMipMaps;
};

struct VideoTextureLoader {
	std::shared_ptr<VideoTexture> load(RenderContext& ctx, const std::string_view& fileName, bool srgb,
		bool generateMipmaps);
};

