#pragma once
#include <volk.h>

class RenderContext;

class Framebuffer final {
	public:
		explicit Framebuffer(RenderContext&, VkFramebuffer);
		~Framebuffer();

		Framebuffer(Framebuffer&&);
		Framebuffer& operator=(Framebuffer&&);

		Framebuffer(const Framebuffer&) = delete;
		void operator=(const Framebuffer&) = delete;

		operator VkFramebuffer() const;

		VkFramebuffer get_framebuffer() const;
	private:
		VkFramebuffer m_framebuffer;
		RenderContext* m_context;
};

