#pragma once

#include <core/common.hpp>

#include <volk.h>

#include <vector>
#include <memory>

class RenderContext;
class Image;
class ImageView;
class Framebuffer;

class RenderPass final {
	public:
		explicit RenderPass(VkRenderPass);
		~RenderPass();

		RenderPass(RenderPass&&);
		RenderPass& operator=(RenderPass&&);

		RenderPass(const RenderPass&) = delete;
		void operator=(RenderPass&) = delete;

		operator VkRenderPass() const;

		VkRenderPass get_render_pass() const;
	private:
		VkRenderPass m_renderPass;
};

class RenderPassBuilder;

class SubpassBuilder final {
	public:
		explicit SubpassBuilder(RenderPassBuilder&, VkPipelineBindPoint);

		SubpassBuilder(SubpassBuilder&&) = default;
		SubpassBuilder& operator=(SubpassBuilder&&) = default;

		SubpassBuilder(const SubpassBuilder&) = delete;
		void operator=(const SubpassBuilder&) = delete;

		SubpassBuilder& add_color_attachment(uint32_t attachmentIndex);
		SubpassBuilder& add_depth_stencil_attachment(uint32_t attachmentIndex);
		SubpassBuilder& add_read_only_depth_stencil_attachment(uint32_t attachmentIndex);
		SubpassBuilder& add_input_attachment(uint32_t attachmentIndex);

		SubpassBuilder& next_subpass(VkPipelineBindPoint bindPoint
				= VK_PIPELINE_BIND_POINT_GRAPHICS);

		RenderPassBuilder& end_subpass();

		bool get_attachment_source_info(uint32_t attachmentIndex,
				VkPipelineStageFlags& srcStageMask, VkAccessFlags& srcAccessMask) const;
		bool has_read_only_depth() const;
	private:
		RenderPassBuilder* m_parent;
		VkPipelineBindPoint m_bindPoint;
		std::vector<VkAttachmentReference> m_colorAttachments;
		std::vector<VkAttachmentReference> m_inputAttachments;
		VkAttachmentReference m_depthAttachment;
		bool m_hasDepthAttachment;

		friend class RenderPassBuilder;
};

class RenderPassBuilder final {
	public:
		explicit RenderPassBuilder() = default;

		NULL_COPY_AND_ASSIGN(RenderPassBuilder);

		RenderPassBuilder& add_attachment(std::shared_ptr<Image>, VkAttachmentLoadOp loadOp,
				VkAttachmentStoreOp storeOp, VkImageLayout finalLayout,
				VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE);
		RenderPassBuilder& add_swapchain_attachment(VkAttachmentLoadOp loadOp,
				VkAttachmentStoreOp storeOp,
				VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE);

		SubpassBuilder& begin_subpass(VkPipelineBindPoint bindPoint
				= VK_PIPELINE_BIND_POINT_GRAPHICS);

		std::shared_ptr<RenderPass> build();
	private:
		std::vector<VkAttachmentDescription> m_attachments;
		std::vector<SubpassBuilder> m_subpasses;

		void build_subpasses(std::vector<VkSubpassDescription>&,
				std::vector<VkSubpassDependency>&);
};

