#include "image_view.hpp"

#include <rendering/render_context.hpp>

ImageView::ImageView(RenderContext& context, VkImageView imageView)
		: m_imageView(imageView)
		, m_context(&context) {}

ImageView::~ImageView() {
	if (m_imageView != VK_NULL_HANDLE) {
		m_context->queue_delete([context=this->m_context,
				imageView=this->m_imageView] {
			vkDestroyImageView(context->get_device(), imageView, nullptr);
		});
	}
}

void ImageView::delete_late() {
	if (m_imageView != VK_NULL_HANDLE) {
		m_context->queue_delete_late([context=this->m_context,
				imageView=this->m_imageView] {
			vkDestroyImageView(context->get_device(), imageView, nullptr);
		});

		m_imageView = VK_NULL_HANDLE;
	}
}

ImageView::operator VkImageView() const {
	return m_imageView;
}

VkImageView ImageView::get_image_view() const {
	return m_imageView;
}

