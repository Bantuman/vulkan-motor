#include "dds_texture.hpp"

#include <cstring>
#include <core/logging.hpp>

#define MAKEFOURCC(a, b, c, d)														\
				((uint32_t)(uint8_t)(a) | ((uint32_t)(uint8_t)(b) << 8) |			\
				((uint32_t)(uint8_t)(c) << 16) | ((uint32_t)(uint8_t)(d) << 24 ))

#define MAKEFOURCCDXT(a) MAKEFOURCC('D', 'X', 'T', a)

#define FOURCC_DXT1 MAKEFOURCCDXT('1')
#define FOURCC_DXT2 MAKEFOURCCDXT('2')
#define FOURCC_DXT3 MAKEFOURCCDXT('3')
#define FOURCC_DXT4 MAKEFOURCCDXT('4')
#define FOURCC_DXT5 MAKEFOURCCDXT('5')

#define FOURCC_R16F 0x0000006F
#define FOURCC_G16R16F 0x00000070
#define FOURCC_A16B16G16R16F 0x00000071
#define FOURCC_R32F 0x00000072
#define FOURCC_G32R32F 0x00000073
#define FOURCC_A32B32G32R32F 0x00000074

// caps1
#define DDSCAPS_COMPLEX				0x00000008 
#define DDSCAPS_TEXTURE				0x00001000 
#define DDSCAPS_MIPMAP				0x00400000 

// caps2
#define DDSCAPS2_CUBEMAP			0x00000200 
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400 
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800 
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000 
#define DDSCAPS2_CUBEMAP_NEGATIVEY	0x00002000 
#define DDSCAPS2_CUBEMAP_POSITIVEZ	0x00004000 
#define DDSCAPS2_CUBEMAP_NEGATIVEZ	0x00008000 
#define DDSCAPS2_VOLUME				0x00200000 

// pixel format flags
#define DDPF_ALPHAPIXELS			0x1
#define DDPF_ALPHA					0x2
#define DDPF_FOURCC					0x4
#define DDPF_RGB					0x40
#define DDPF_YUV					0x200
#define DDPF_LUMINANCE				0x20000

struct Header {
	uint32_t size;
	uint32_t flags;
	uint32_t height;
	uint32_t width;
	uint32_t pitchOrLinearSize;
	uint32_t depth;
	uint32_t mipMapCount;
	uint32_t reserved1[11];
	struct PixelFormat {
		uint32_t size;
		uint32_t flags;
		uint32_t fourCC;
		uint32_t rgbBitCount;
		uint32_t rBitMask;
		uint32_t gBitMask;
		uint32_t bBitMask;
	} pixelFormat;
	uint32_t caps;
	uint32_t caps2;
	uint32_t caps3;
	uint32_t caps4;
	uint32_t reserved2;
};

using namespace DDS;

static void calc_size(const Header&, DDSTextureInfo&);
static void get_format(const Header&, DDSTextureInfo&, bool);
static bool is_compressed(const Header&);

bool DDS::get_texture_info(DDSTextureInfo& textureInfo, const void* memory, size_t size,
		bool srgb) {
	if (size < 128) {
		return false;
	}

	const uint8_t* bytes = reinterpret_cast<const uint8_t*>(memory);

	if (strncmp(reinterpret_cast<const char*>(bytes), "DDS ", 4) != 0) {
		return false;
	}

	const Header& header = *reinterpret_cast<const Header*>(bytes + 4);

	textureInfo.height = header.height;
	textureInfo.width = header.width;
	textureInfo.mipMapCount = header.mipMapCount;

	calc_size(header, textureInfo);
	get_format(header, textureInfo, srgb);

	textureInfo.hasMipMaps = header.caps & DDSCAPS_MIPMAP;
	textureInfo.isCubeMap = header.caps2 & DDSCAPS2_CUBEMAP;
	textureInfo.isCompressed = is_compressed(header);
	textureInfo.hasAlpha = header.pixelFormat.flags & DDPF_ALPHAPIXELS;

	textureInfo.dataStart = bytes + 128;

	return true;
}

static void get_format(const Header& header, DDSTextureInfo& textureInfo, bool srgb) {
	switch (header.pixelFormat.fourCC) {
		case FOURCC_DXT1:
			textureInfo.format = textureInfo.hasAlpha ? VK_FORMAT_BC1_RGBA_UNORM_BLOCK
					: VK_FORMAT_BC1_RGB_UNORM_BLOCK;
			break;
		case FOURCC_DXT3:
			textureInfo.format = VK_FORMAT_BC2_UNORM_BLOCK;
			break;
		case FOURCC_DXT5:
			textureInfo.format = VK_FORMAT_BC3_UNORM_BLOCK;
			break;
		case FOURCC_R16F:
			textureInfo.format = VK_FORMAT_R16_SFLOAT;
			break;
		case FOURCC_G16R16F:
			textureInfo.format = VK_FORMAT_R16G16_SFLOAT;
			break;
		case FOURCC_A16B16G16R16F:
			textureInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			break;
		case FOURCC_R32F:
			textureInfo.format = VK_FORMAT_R32_SFLOAT;
			break;
		case FOURCC_G32R32F:
			textureInfo.format = VK_FORMAT_R32G32_SFLOAT;
			break;
		case FOURCC_A32B32G32R32F:
			textureInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
			break;
		default:
			textureInfo.format = VK_FORMAT_UNDEFINED;
	}

	if (srgb && textureInfo.format != VK_FORMAT_UNDEFINED) {
		textureInfo.format = static_cast<VkFormat>(static_cast<uint32_t>(textureInfo.format) + 1);
	}
}

static bool is_compressed(const Header& header) {
	if ((header.pixelFormat.flags & DDPF_FOURCC) == 0) {
		return false;
	}

	switch (header.pixelFormat.fourCC) {
		case FOURCC_DXT1:
		case FOURCC_DXT3:
		case FOURCC_DXT5:
			return true;
		default:
			return false;
	}
}

static void calc_size(const Header& header, DDSTextureInfo& textureInfo) {
	uint32_t size;

	if (is_compressed(header)) {
		size = header.pitchOrLinearSize;
	}
	else {
		size = textureInfo.width * textureInfo.height;

		switch (header.pixelFormat.fourCC) {
			case FOURCC_A16B16G16R16F:
				size *= 8;
				break;
			case FOURCC_A32B32G32R32F:
				size *= 16;
				break;
		}
	}

	if (textureInfo.hasMipMaps) {
		size = size * 3 / 2;
	}

	if (textureInfo.isCubeMap) {
		size *= 6;
	}

	textureInfo.size = size;
}

