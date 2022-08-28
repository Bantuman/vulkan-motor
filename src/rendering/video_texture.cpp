#include "video_texture.hpp"

#include <file/file_system.hpp>
#include <path_utils.hpp>
#include <rendering/vk_initializers.hpp>
#include <logging.hpp>
#include <cmath>

#define RET_EMPTY(a) \
if(a){ \
LOG_ERROR("video_texture", "%s", #a); \
return {};\
}

#define CALC_FFMPEG_VERSION(a, b, c) ( a << 16 | b << 8 | c )

double r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

double get_real_fps(AVFormatContext * aContext, uint16_t aStream)
{
	double eps_zero = 0.000025;
	double fps = r2d(aContext->streams[aStream]->r_frame_rate);

#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(52, 111, 0)
	if (fps < eps_zero)
	{
		fps = r2d(aContext->streams[aStream]->avg_frame_rate);
	}
#endif

	if (fps < eps_zero)
	{
		fps = 1.0 / r2d(aContext->streams[aStream]->codec->time_base);
	}

	return fps;
}


std::shared_ptr<Image> VideoTexture::get_image() const {
	return m_image;
}

std::shared_ptr<Buffer> VideoTexture::get_buffer() const
{
	return m_buffer;
}

std::shared_ptr<ImageView> VideoTexture::get_image_view() const {
	return m_imageView;
}

uint32_t VideoTexture::get_num_mip_maps() const {
	return m_numMipMaps;
}

FrameStatus VideoTexture::iterate_frame()
{
	int valid = false;
	int readFrame = 0;
//	int safeValue = 0;
	//const int maxSafeVal = 10000;

	while (!valid /* || safeValue > maxSafeVal*/)
	{
		//safeValue++;
		readFrame = av_read_frame(m_videoFormatContext, &m_avPacket);
		if (readFrame >= 0)
		{
			if (m_avPacket.stream_index == m_streamIndex)
			{
				m_decodedBytes = avcodec_decode_video2(
					m_videoCodecContext,
					m_avVideoFrame,
					&valid,//&myGotFrame,
					&m_avPacket);
			}
			else if (/*FIXME: Implement audio*/ false)
			{
			}
		}
		else
		{
			valid = true;
		}
		av_free_packet(&m_avPacket);
	}

	/*if (safeValue >= maxSafeVal)
	{
		RET_EMPTY("Video error: Could not find a valid video frame!");
	}*/

	return readFrame < 0 ? FrameStatus::END_OR_ERROR : FrameStatus::ITERATE;
}

uint16_t prev_width;
uint16_t prev_height;
void VideoTexture::write_image(RenderContext& ctx, uint16_t width, uint16_t height)
{
	if (m_swsVideoContext)
	{
		auto result = sws_scale(
			m_swsVideoContext,
			m_avVideoFrame->data,
			m_avVideoFrame->linesize,
			0,
			m_videoCodecContext->height,
			m_avVideoFrameBGR->data,
			m_avVideoFrameBGR->linesize);

		if (result > 0)
		{
			int* buffer = static_cast<int*>(m_buffer->map());
			uint8_t* data = m_avVideoFrameBGR->data[0];
			memcpy_s(buffer, m_numberBytes * sizeof(uint8_t), data, m_numberBytes * sizeof(uint8_t));
			m_buffer->unmap();
		}
	}
	prev_width = width;
	prev_height = height;
	m_frameCount++;
}

double VideoTexture::get_fps() const
{
	return get_real_fps(m_videoFormatContext, m_streamIndex);
}

// FIXME: Implement
float VideoTexture::get_milliseconds() const
{
	return 0.0f;
}

// FIXME: Implement
void VideoTexture::set_milliseconds(float)
{
}

uint32_t VideoTexture::get_frame() const
{
	return m_frameCount;
}

void VideoTexture::set_frame(int64_t timestamp)
{
	av_seek_frame(m_videoFormatContext, m_streamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
}

std::shared_ptr<VideoTexture> VideoTextureLoader::load(RenderContext& ctx, const std::string_view& fileName, bool srgb, bool generateMipmaps) {
	std::shared_ptr<VideoTexture> video = std::make_shared<VideoTexture>();
	AVFormatContext*& videoFormatContext = video->m_videoFormatContext;
	
	av_register_all();
	auto result = avformat_open_input(&videoFormatContext, g_fileSystem->get_file_system_path(fileName).data(), NULL, NULL);
	RET_EMPTY(result < 0);

	result = avformat_find_stream_info(videoFormatContext, NULL);
	RET_EMPTY(result < 0);

	for (uint16_t i = 0; i < videoFormatContext->nb_streams; ++i)
	{
		if (videoFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video->m_streamIndex = i;
			video->m_videoCodecContext = videoFormatContext->streams[i]->codec;
			if (video->m_videoCodecContext)
				video->m_videoCodec = avcodec_find_decoder(video->m_videoCodecContext->codec_id);
		}
		else if(/*FIXME: Implement audio*/ false)
		{

		}
	}

	if (AVCodecContext* videoContext = video->m_videoCodecContext; video->m_videoCodec)
	{
		result = avcodec_open2(videoContext, video->m_videoCodec, NULL);
		RET_EMPTY(result < 0);
		video->m_avVideoFrame = avcodec_alloc_frame();
		video->m_avVideoFrameBGR = avcodec_alloc_frame();

		AVPixelFormat format = AV_PIX_FMT_RGBA;
		video->m_numberBytes = avpicture_get_size(format, videoContext->width, videoContext->height);
		video->m_buffer = ctx.buffer_create(video->m_numberBytes * sizeof(uint8_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
		video->m_videoBuffer = reinterpret_cast<uint8_t*>(av_malloc(video->m_numberBytes * sizeof(uint8_t))); //reinterpret_cast<uint8_t*>(video->m_buffer->map());//
		avpicture_fill(reinterpret_cast<AVPicture*>(video->m_avVideoFrameBGR), video->m_videoBuffer, format, videoContext->width, videoContext->height);
	//	video->m_buffer->unmap();
		video->m_swsVideoContext = sws_getContext(
			videoContext->width,
			videoContext->height,
			videoContext->pix_fmt,
			videoContext->width,
			videoContext->height,
			format,
			SWS_BILINEAR,
			NULL,
			NULL,
			NULL
		);
	}
	else
	{
		RET_EMPTY("m_videoCodecContext or m_videoCodec was nullptr");
	}

	VkExtent3D extents = {
		static_cast<uint32_t>(video->m_videoCodecContext->width),
		static_cast<uint32_t>(video->m_videoCodecContext->height),
		1
	};

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	int usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if (generateMipmaps) {
		usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	auto createInfo = vkinit::image_create_info(format, usageFlags, extents);
	createInfo.mipLevels = 1;

	auto image = ctx.image_create(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);

	if (!image) {
		av_free(video->m_videoBuffer);
		RET_EMPTY("!image");
	}

	auto imageView = ctx.image_view_create(image, VK_IMAGE_VIEW_TYPE_2D, format,
		VK_IMAGE_ASPECT_COLOR_BIT, 1);

	if (!imageView) {
		av_free(video->m_videoBuffer);
		RET_EMPTY("!imageView");
	}

	video->m_imageView = imageView;
	video->m_image = image;

	return video;
}

