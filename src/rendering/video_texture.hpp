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

struct VideoTextureLoader;

class VideoTexture {
	enum class FrameStatus
	{
		ITERATE = 0,
		END_OR_ERROR = 1
	};
	friend class VideoTextureLoader;
public:
	explicit VideoTexture(std::shared_ptr<Image> video, std::shared_ptr<ImageView> imageView,
		uint32_t numMipMaps);

	std::shared_ptr<Image> get_image() const;
	std::shared_ptr<ImageView> get_image_view() const;
	uint32_t get_num_mip_maps() const;

	FrameStatus iterate_frame();
	void write_image(RenderContext& ctx, uint16_t width, uint16_t height);

	double get_fps() const;

	float get_milliseconds() const;
	void set_milliseconds(float milliseconds);
	uint32_t get_frame() const;
	void set_frame(int64_t);

private:
	AVCodec* m_videoCodec;

	AVCodecContext* m_videoCodecContext;
	AVCodecContext* m_audioCodecContext;
	SwsContext* m_swsVideoContext;
	SwrContext* m_swsAudioContext;
	AVFormatContext* m_videoFormatContext;

	AVFrame* m_avVideoFrame;
	AVFrame* m_avVideoFrameBGR;
	AVFrame* m_avAudioFrame;
	uint8_t* m_videoBuffer;
	AVPacket m_avPacket;

	std::shared_ptr<Buffer> m_buffer;
	std::shared_ptr<Image> m_image;
	std::shared_ptr<ImageView> m_imageView;
	uint32_t m_numMipMaps;
	uint32_t m_frameCount;
	uint32_t m_decodedBytes;
	uint32_t m_numberBytes;
	uint16_t m_streamIndex;
};

struct VideoTextureLoader {
	std::shared_ptr<VideoTexture> load(RenderContext& ctx, const std::string_view& fileName, bool srgb,
		bool generateMipmaps);
};

