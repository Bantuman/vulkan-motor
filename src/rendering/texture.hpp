#pragma once

#include <rendering/render_context.hpp>

class Texture {
	public:
		explicit Texture(std::shared_ptr<Image> image, std::shared_ptr<ImageView> imageView,
				uint32_t numMipMaps);

		std::shared_ptr<Image> get_image() const;
		std::shared_ptr<ImageView> get_image_view() const;
		uint32_t get_num_mip_maps() const;
	private:
		std::shared_ptr<Image> m_image;
		std::shared_ptr<ImageView> m_imageView;
		uint32_t m_numMipMaps;
};

struct TextureLoader {
	std::shared_ptr<Texture> load(RenderContext& ctx, const std::string_view& fileName, bool srgb,
			bool generateMipmaps);
};

