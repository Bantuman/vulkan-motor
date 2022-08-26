#pragma once

#include <rendering/render_context.hpp>

class CubeMap {
	public:
		explicit CubeMap(std::shared_ptr<Image> image, std::shared_ptr<ImageView> imageView);

		std::shared_ptr<Image> get_image() const;
		std::shared_ptr<ImageView> get_image_view() const;
	private:
		std::shared_ptr<Image> m_image;
		std::shared_ptr<ImageView> m_imageView;
};

struct CubeMapLoader {
	std::shared_ptr<CubeMap> load(RenderContext& ctx, const std::string_view* fileNames,
			size_t numFileNames, bool srgb);
};

