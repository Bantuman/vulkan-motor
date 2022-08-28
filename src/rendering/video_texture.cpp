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

VideoTexture::FrameStatus VideoTexture::iterate_frame()
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
			int* data = static_cast<int*>(m_buffer->map());
			for (uint32_t x = m_videoCodecContext->height; x < width; ++x)
			{
				for (uint32_t y = m_videoCodecContext->width; y < height; ++y)
				{
					data[x * width + y] = 0;
				}
			}

			int rgbIndex = 0;
			for (uint32_t x = 0; x < m_videoCodecContext->height; ++x)
			{
				for (uint32_t y = 0; y < m_videoCodecContext->width; ++y)
				{
					{
						uint8_t* data = m_avVideoFrameBGR->data[0];
						data[x * width + y] = int((data[rgbIndex + 3]) << 24 |
							(data[rgbIndex + 2]) << 16 |
							(data[rgbIndex + 1]) << 8 |
							(data[rgbIndex]));

						rgbIndex += 4;
					}
				}
			}
			m_buffer->unmap();
		}
	}

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
	std::vector<char> data = g_fileSystem->file_read_bytes(fileName);
	RET_EMPTY(data.empty());
	
	std::shared_ptr<VideoTexture> video = std::make_shared<VideoTexture>();
	AVFormatContext*& videoFormatContext = video->m_videoFormatContext;

	av_register_all();
	auto result = avformat_open_input(&videoFormatContext, fileName.data(), NULL, NULL);
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
		video->m_videoBuffer = reinterpret_cast<uint8_t*>(av_malloc(video->m_numberBytes * sizeof(uint8_t)));
		avpicture_fill(reinterpret_cast<AVPicture*>(video->m_avVideoFrameBGR), video->m_videoBuffer, format, videoContext->width, videoContext->height);

		//m_buffer = ctx.buffer_create(video->m_numberBytes * sizeof(uint8_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
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
	return video;
}

