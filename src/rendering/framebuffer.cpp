#include "framebuffer.hpp"

#include <rendering/render_context.hpp>

Framebuffer::Framebuffer(RenderContext& context, VkFramebuffer framebuffer)
		: m_framebuffer(framebuffer)
		, m_context(&context) {}

Framebuffer::~Framebuffer() {
	if (m_framebuffer != VK_NULL_HANDLE) {
		m_context->queue_delete([device=this->m_context->get_device(),
				framebuffer=this->m_framebuffer] {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		});
	}
}

Framebuffer::Framebuffer(Framebuffer&& other)
		: m_framebuffer(other.m_framebuffer)
		, m_context(other.m_context) {
	other.m_framebuffer = VK_NULL_HANDLE;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) {
	m_framebuffer = other.m_framebuffer;
	m_context = other.m_context;

	other.m_framebuffer = VK_NULL_HANDLE;

	return *this;
}

Framebuffer::operator VkFramebuffer() const {
	return m_framebuffer;
}

VkFramebuffer Framebuffer::get_framebuffer() const {
	return m_framebuffer;
}

