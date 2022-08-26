#pragma once

#include <volk.h>

class RenderContext;

class Sampler {
	public:
		explicit Sampler(RenderContext&, VkSampler);
		~Sampler();

		Sampler(Sampler&&);
		Sampler& operator=(Sampler&&);

		Sampler(const Sampler&) = delete;
		void operator=(const Sampler&) = delete;

		operator VkSampler() const;

		VkSampler get_sampler() const;
	private:
		VkSampler m_sampler;
		RenderContext* m_context;
};

