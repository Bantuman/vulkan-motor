#include "sampler.hpp"

#include <rendering/render_context.hpp>

Sampler::Sampler(RenderContext& ctx, VkSampler sampler)
		: m_sampler(sampler)
		, m_context(&ctx) {}

Sampler::~Sampler() {
	if (m_sampler != VK_NULL_HANDLE) {
		m_context->queue_delete([device=this->m_context->get_device(), sampler=this->m_sampler] {
			vkDestroySampler(device, sampler, nullptr);
		});
	}
}

Sampler::Sampler(Sampler&& other)
		: m_sampler(other.m_sampler)
		, m_context(other.m_context) {
	other.m_sampler = VK_NULL_HANDLE;
}

Sampler& Sampler::operator=(Sampler&& other) {
	m_sampler = other.m_sampler;
	m_context = other.m_context;

	other.m_sampler = VK_NULL_HANDLE;
	
	return *this;
}

Sampler::operator VkSampler() const {
	return m_sampler;
}

VkSampler Sampler::get_sampler() const {
	return m_sampler;
}

