#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

class RenderContext;

class ImageView final {
	public:
		explicit ImageView(RenderContext&, VkImageView);
		~ImageView();

		void delete_late();

		operator VkImageView() const;

		VkImageView get_image_view() const;
	private:
		VkImageView m_imageView;
		RenderContext* m_context;
};

