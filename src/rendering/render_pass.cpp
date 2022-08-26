#include "render_pass.hpp"

#include <rendering/render_context.hpp>

static void generate_dependencies(std::vector<VkSubpassDependency>& dependencies,
		uint32_t dstPassIndex, const SubpassBuilder& srcPass, uint32_t srcPassIndex,
		uint32_t attachRefCount, const VkAttachmentReference* pAttachRefs,
		VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask);

// RenderPass

RenderPass::RenderPass(VkRenderPass renderPass)
		: m_renderPass(renderPass) {}

RenderPass::~RenderPass() {
	if (m_renderPass != VK_NULL_HANDLE) {
		g_renderContext->queue_delete([renderPass=m_renderPass] {
			vkDestroyRenderPass(g_renderContext->get_device(), renderPass, nullptr);
		});
	}
}

RenderPass::RenderPass(RenderPass&& other)
		: m_renderPass(other.m_renderPass) {
	other.m_renderPass = VK_NULL_HANDLE;
}

RenderPass& RenderPass::operator=(RenderPass&& other) {
	m_renderPass = other.m_renderPass;
	other.m_renderPass = VK_NULL_HANDLE;

	return *this;
}

RenderPass::operator VkRenderPass() const {
	return m_renderPass;
}

VkRenderPass RenderPass::get_render_pass() const {
	return m_renderPass;
}

// SubpassBuilder

SubpassBuilder::SubpassBuilder(RenderPassBuilder& parent, VkPipelineBindPoint bindPoint)
		: m_parent(&parent)
		, m_bindPoint(bindPoint)
		, m_hasDepthAttachment(false) {}

SubpassBuilder& SubpassBuilder::add_color_attachment(uint32_t attachmentIndex) {
	m_colorAttachments.push_back({attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
	return *this;
}

SubpassBuilder& SubpassBuilder::add_depth_stencil_attachment(uint32_t attachmentIndex) {
	m_hasDepthAttachment = true;
	m_depthAttachment = {attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

	return *this;
}

SubpassBuilder& SubpassBuilder::add_read_only_depth_stencil_attachment(uint32_t attachmentIndex) {
	m_hasDepthAttachment = true;
	m_depthAttachment = {attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};

	return *this;
}

SubpassBuilder& SubpassBuilder::add_input_attachment(uint32_t attachmentIndex) {
	m_inputAttachments.push_back({attachmentIndex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
	return *this;
}

SubpassBuilder& SubpassBuilder::next_subpass(VkPipelineBindPoint bindPoint) {
	return m_parent->begin_subpass(bindPoint);
}

RenderPassBuilder& SubpassBuilder::end_subpass() {
	return *m_parent;
}

bool SubpassBuilder::get_attachment_source_info(uint32_t attachmentIndex,
		VkPipelineStageFlags &srcStageMask, VkAccessFlags &srcAccessMask) const {
	for (auto& attachRef : m_colorAttachments) {
		if (attachRef.attachment == attachmentIndex) {
			srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			return true;
		}
	}

	// FIXME: should I check input attachments?

	if (m_hasDepthAttachment && m_depthAttachment.attachment == attachmentIndex) {
		srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
				| VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

		if (has_read_only_depth()) {
			srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}

		return true;
	}

	return false;
}

bool SubpassBuilder::has_read_only_depth() const {
	return m_hasDepthAttachment
			&& m_depthAttachment.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
}

// RenderPassBuilder

RenderPassBuilder& RenderPassBuilder::add_attachment(std::shared_ptr<Image> image,
		VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkImageLayout finalLayout,
		VkImageLayout initialLayout, VkAttachmentLoadOp stencilLoadOp,
		VkAttachmentStoreOp stencilStoreOp) {
	VkAttachmentDescription attach{};
	attach.format = image->get_format();
	attach.samples = image->get_sample_count();
	attach.loadOp = loadOp;
	attach.storeOp = storeOp;
	attach.stencilLoadOp = stencilLoadOp;
	attach.stencilStoreOp = stencilStoreOp;
	attach.initialLayout = initialLayout;
	attach.finalLayout = finalLayout;

	m_attachments.emplace_back(std::move(attach));

	return *this;
}

RenderPassBuilder& RenderPassBuilder::add_swapchain_attachment(VkAttachmentLoadOp loadOp,
		VkAttachmentStoreOp storeOp, VkAttachmentLoadOp stencilLoadOp,
		VkAttachmentStoreOp stencilStoreOp) {
	VkAttachmentDescription attach{};
	attach.format = g_renderContext->get_swapchain_image_format();
	attach.samples = VK_SAMPLE_COUNT_1_BIT;
	attach.loadOp = loadOp;
	attach.storeOp = storeOp;
	attach.stencilLoadOp = stencilLoadOp;
	attach.stencilStoreOp = stencilStoreOp;
	attach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	m_attachments.emplace_back(std::move(attach));

	return *this;
}

SubpassBuilder& RenderPassBuilder::begin_subpass(VkPipelineBindPoint bindPoint) {
	m_subpasses.emplace_back(*this, bindPoint);
	return m_subpasses.back();
}

std::shared_ptr<RenderPass> RenderPassBuilder::build() {
	VkRenderPassCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	if (!m_attachments.empty()) {
		createInfo.attachmentCount = static_cast<uint32_t>(m_attachments.size());
		createInfo.pAttachments = m_attachments.data();
	}

	std::vector<VkSubpassDescription> subpasses(m_subpasses.size());
	std::vector<VkSubpassDependency> dependencies;

	build_subpasses(subpasses, dependencies);

	createInfo.subpassCount = static_cast<uint32_t>(m_subpasses.size());
	createInfo.pSubpasses = subpasses.data();

	if (!dependencies.empty()) {
		createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		createInfo.pDependencies = dependencies.data();
	}

	VkRenderPass renderPass;
	if (vkCreateRenderPass(g_renderContext->get_device(), &createInfo, nullptr, &renderPass)
			!= VK_SUCCESS) {
		return nullptr;
	}

	return std::make_shared<RenderPass>(renderPass);
}

void RenderPassBuilder::build_subpasses(std::vector<VkSubpassDescription>& subpasses,
		std::vector<VkSubpassDependency>& dependencies) {
	for (uint32_t i = 0; i < m_subpasses.size(); ++i) {
		auto& passInfo = m_subpasses[i];
		auto& desc = subpasses[i];

		desc.pipelineBindPoint = passInfo.m_bindPoint;
		desc.inputAttachmentCount = static_cast<uint32_t>(passInfo.m_inputAttachments.size());
		desc.pInputAttachments = passInfo.m_inputAttachments.data();
		desc.colorAttachmentCount = static_cast<uint32_t>(passInfo.m_colorAttachments.size());
		desc.pColorAttachments = passInfo.m_colorAttachments.data();

		if (passInfo.m_hasDepthAttachment) {
			desc.pDepthStencilAttachment = &passInfo.m_depthAttachment;
		}

		for (uint32_t j = 0; j < i; ++j) {
			auto& precedingPass = m_subpasses[j];

			generate_dependencies(dependencies, i, precedingPass, j,
					passInfo.m_inputAttachments.size(), passInfo.m_inputAttachments.data(),
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

			generate_dependencies(dependencies, i, precedingPass, j,
					passInfo.m_colorAttachments.size(), passInfo.m_colorAttachments.data(),
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

			if (passInfo.m_hasDepthAttachment) {
				VkAccessFlags dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

				if (!passInfo.has_read_only_depth()) {
					dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				}

				generate_dependencies(dependencies, i, precedingPass, j, 1,
						&passInfo.m_depthAttachment, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
						| VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, dstAccessMask);
			}
		}
	}
}

static void generate_dependencies(std::vector<VkSubpassDependency>& dependencies,
		uint32_t dstPassIndex, const SubpassBuilder& srcPass, uint32_t srcPassIndex,
		uint32_t attachRefCount, const VkAttachmentReference* pAttachRefs,
		VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask) {
	for (uint32_t i = 0; i < attachRefCount; ++i) {
		auto attachIndex = pAttachRefs[i].attachment;
		VkPipelineStageFlags srcStageMask;
		VkAccessFlags srcAccessMask;

		if (srcPass.get_attachment_source_info(attachIndex, srcStageMask, srcAccessMask)) {
			VkSubpassDependency dep{};
			dep.srcSubpass = srcPassIndex;
			dep.dstSubpass = dstPassIndex;
			dep.srcStageMask = srcStageMask;
			dep.srcAccessMask = srcAccessMask;
			dep.dstStageMask = dstStageMask;
			dep.dstAccessMask = dstAccessMask;
			dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies.emplace_back(std::move(dep));

			break;
		}
	}
}

