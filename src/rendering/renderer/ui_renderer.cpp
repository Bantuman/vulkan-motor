#include "ui_renderer.hpp"

#include <cassert>

#include <core/hashed_string.hpp>
#include <ui/frame.hpp>
#include <ui/text_label.hpp>
#include <core/logging.hpp>

#include <ecs/ecs.hpp>

#include <rendering/render_context.hpp>
#include <rendering/render_pass.hpp>
#include <rendering/shader_program.hpp>
#include <rendering/text_bitmap.hpp>
#include <rendering/vk_initializers.hpp>

static constexpr size_t INITIAL_CAPACITY = 16;

// UIRenderer

UIRenderer::UIRenderer(TextBitmap& textBitmap, VkRenderPass renderPass, uint32_t subpassIndex)
		: m_textBitmap(textBitmap) {
	init_pipelines(renderPass, subpassIndex);
	init_buffers();
	init_images();
	init_descriptors();

	g_ecs->get_group<Game::RectInstance, Game::RectInstanceInfo>();
}

/*void UIRenderer::sort() {
	assert(m_numRects == m_priorityInfo.size());

	for (size_t i = 0; i < m_numRects; ++i) {
		size_t j = i;

		while (j > 0 && m_priorityInfo[j] < m_priorityInfo[j - 1]) {
			std::swap(m_priorityInfo[j], m_priorityInfo[j - 1]);
			std::swap(m_rectInstMapping[j], m_rectInstMapping[j - 1]);
			--j;
		}
	}
}*/

uint32_t UIRenderer::get_image_index(VkImageView imageView) {
	if (auto it = m_imageIndices.find(imageView); it != m_imageIndices.end()) {
		return it->second;
	}

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageView = imageView;
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	uint32_t newIndex = static_cast<uint32_t>(m_boundImageInfo.size());

	m_boundImageInfo.emplace_back(std::move(imgInfo));
	m_neededDescriptorUpdates = RenderContext::FRAMES_IN_FLIGHT;

	m_imageIndices.emplace(std::make_pair(imageView, newIndex));

	return newIndex;
}

TextBitmap& UIRenderer::get_text_bitmap() {
	return m_textBitmap;
}

void UIRenderer::update(CommandBuffer& cmd) {
	if (m_gpuRectInstances->get_size() == 0) {
		return;
	}

	fetch_rect_updates();

	auto& pool = g_ecs->get_or_create_pool<Game::RectInstance>();

	std::vector<VkBufferCopy> copies;
	std::vector<VkBufferMemoryBarrier> barriers;

	if (m_gpuRectInstances->get_size() < pool.size() * sizeof(Game::RectInstance)) {
		m_rectUpdateRanges.clear();

		m_gpuRectInstances = g_renderContext->buffer_create(
				pool.capacity() * sizeof(Game::RectInstance),
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);

		VkBufferCopy copy{};
		copy.srcOffset = 0;
		copy.dstOffset = 0;
		copy.size = pool.size() * sizeof(Game::RectInstance);

		VkBufferMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		barrier.buffer = *m_gpuRectInstances;
		barrier.offset = 0;
		barrier.size = copy.size;

		copies.emplace_back(std::move(copy));
		barriers.emplace_back(std::move(barrier));
	}
	else if (!m_rectUpdateRanges.empty()) {
	/*	VkBufferCopy copy{};
		copy.srcOffset = copy.dstOffset = 0;
		copy.size = pool.size() * sizeof(Game::RectInstance);

		VkBufferMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		barrier.buffer = *m_gpuRectInstances;
		barrier.offset = 0;
		barrier.size = copy.size;

		copies.emplace_back(std::move(copy));
		barriers.emplace_back(std::move(barrier));*/
		for (auto& rng : m_rectUpdateRanges) {
			VkBufferCopy copy{};
			copy.srcOffset = copy.dstOffset = rng.min * sizeof(Game::RectInstance);
			copy.size = (rng.max - rng.min) * sizeof(Game::RectInstance);

			VkBufferMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			barrier.buffer = *m_gpuRectInstances;
			barrier.offset = copy.dstOffset;
			barrier.size = copy.size;

			copies.emplace_back(std::move(copy));
			barriers.emplace_back(std::move(barrier));
		}

		m_rectUpdateRanges.clear();
	}

	if (copies.empty()) {
		return;
	}

	cmd.pipeline_barrier(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
	cmd.copy_buffer(pool.get_buffer(), *m_gpuRectInstances,
			static_cast<uint32_t>(copies.size()), copies.data());
	cmd.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			0, 0, nullptr, static_cast<uint32_t>(barriers.size()), barriers.data(), 0, nullptr);
}

void UIRenderer::render(CommandBuffer& cmd) {
	VkDeviceSize offset = 0;
	//VkBuffer rectBuffer = m_rectInstances->get_buffer();
	//VkBuffer rectBuffer = g_ecs->get_or_create_pool<Game::RectInstance>().get_buffer();
	VkBuffer rectBuffer = *m_gpuRectInstances;
	auto numRects = g_ecs->get_or_create_pool<Game::RectInstance>().size();

	if (m_neededDescriptorUpdates > 0) {
		--m_neededDescriptorUpdates;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = static_cast<uint32_t>(m_boundImageInfo.size());
		write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.pImageInfo = m_boundImageInfo.data();
		write.dstBinding = 1;
		write.dstSet = m_imageDescriptors[g_renderContext->get_frame_index()];

		vkUpdateDescriptorSets(g_renderContext->get_device(), 1, &write, 0, nullptr);
	}

	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->get_layout(),
			0, 1, &m_imageDescriptors[g_renderContext->get_frame_index()], 0, nullptr);

	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
	cmd.bind_vertex_buffers(0, 1, &rectBuffer, &offset);
	cmd.draw(6, static_cast<uint32_t>(numRects), 0, 0);
}

void UIRenderer::mark_rect_for_update(ECS::Entity entity) {
	if (!g_ecs->has_component<Game::RectInstance>(entity))
		return;
	auto& pool = g_ecs->get_pool<Game::RectInstance>();
	m_rectUpdateRanges.add(static_cast<uint32_t>(pool.get_sparse_index(entity)));
}

void UIRenderer::init_pipelines(VkRenderPass renderPass, uint32_t subpassIndex) {
	auto uiRectShader = ShaderProgramBuilder()
		.add_shader("shaders://ui_rect.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
		.add_shader("shaders://ui_rect.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, true)
		.build(*g_renderContext);

	VkVertexInputAttributeDescription attributeDescs[] = {
		{0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Game::RectInstance, texLayout)},
		{1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Game::RectInstance, color)},
		{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Game::RectInstance, transform)},
		{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Game::RectInstance, transform) + 2 * sizeof(float)},
		{4, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Game::RectInstance, transform) + 4 * sizeof(float)},
		{5, 0, VK_FORMAT_R32G32_UINT, offsetof(Game::RectInstance, imageIndex)}
	};

	VkVertexInputBindingDescription bindingDescs[] = {
		{0, sizeof(Game::RectInstance), VK_VERTEX_INPUT_RATE_INSTANCE},
	};

	m_pipeline = PipelineBuilder()
		.set_vertex_attribute_descriptions(attributeDescs, 6)
		.set_vertex_binding_descriptions(bindingDescs, 1)
		.set_blend_enabled(true)
		.set_color_blend(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA,
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
		.set_alpha_blend(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
				VK_BLEND_FACTOR_ONE)
		.add_program(*uiRectShader)
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.build(renderPass, subpassIndex);
}

void UIRenderer::init_buffers() {
	m_gpuRectInstances = g_renderContext->buffer_create(
			INITIAL_CAPACITY * sizeof(Game::RectInstance),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
}

void UIRenderer::init_images() {
	auto samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	m_nearestSampler = g_renderContext->sampler_create(samplerInfo);

	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	m_linearSampler = g_renderContext->sampler_create(samplerInfo);

	auto imageInfo = vkinit::image_create_info(VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VkExtent3D{1, 1, 1});
	m_blankImage = g_renderContext->image_create(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY);
	m_viewBlankImage = g_renderContext->image_view_create(m_blankImage, VK_IMAGE_VIEW_TYPE_2D,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

	g_renderContext->immediate_submit([&](VkCommandBuffer cmd) {
		VkImageMemoryBarrier toTransfer{};
		toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		toTransfer.srcAccessMask = 0;
		toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		toTransfer.image = *m_blankImage;
		toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		toTransfer.subresourceRange.levelCount = 1;
		toTransfer.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransfer);

		VkClearColorValue clearColorValue{};
		clearColorValue.float32[0] = 1.f;
		clearColorValue.float32[1] = 1.f;
		clearColorValue.float32[2] = 1.f;
		clearColorValue.float32[3] = 1.f;

		vkCmdClearColorImage(cmd, *m_blankImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				&clearColorValue, 1, &toTransfer.subresourceRange); 

		VkImageMemoryBarrier toGraphics = toTransfer;
		toGraphics.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		toGraphics.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		toGraphics.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		toGraphics.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toGraphics);
	});
}

void UIRenderer::init_descriptors() {
	VkDescriptorImageInfo blankImg{};
	blankImg.imageView = *m_viewBlankImage;
	blankImg.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	m_boundImageInfo.emplace_back(std::move(blankImg));

	VkDescriptorImageInfo fontImg{};
	fontImg.imageView = m_textBitmap.get_bitmap_image_view();
	fontImg.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	m_boundImageInfo.emplace_back(std::move(fontImg));

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorCount = static_cast<uint32_t>(m_boundImageInfo.size());
	write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write.pImageInfo = m_boundImageInfo.data();
	write.dstBinding = 1;

	VkDescriptorImageInfo samplerInfo[2] = {};
	samplerInfo[0].sampler = *m_linearSampler;
	samplerInfo[1].sampler = *m_nearestSampler;

	for (size_t i = 0; i < RenderContext::FRAMES_IN_FLIGHT; ++i) {
		m_imageDescriptors[i] = g_renderContext->global_descriptor_set_begin()
			.bind_images(0, samplerInfo, 2, VK_DESCRIPTOR_TYPE_SAMPLER,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.bind_dynamic_array(1, 32, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		write.dstSet = m_imageDescriptors[i];

		vkUpdateDescriptorSets(g_renderContext->get_device(), 1, &write, 0, nullptr);
	}
}

void UIRenderer::fetch_rect_updates() {
	// FIXME: frame allocator
	std::vector<size_t> updates;
	// FIXME: Tag rect updates
	auto& group = g_ecs->get_group<Game::RectInstance, Game::RectInstanceInfo>();
	auto& infoPool = g_ecs->get_pool<Game::RectInstanceInfo>();
	const auto* rectInfoData = infoPool.get_components();
	//const auto* dense = infoPool.get_dense().data();

	g_ecs->run_system<Game::RectInstance, Game::RectInstanceInfo, ECS::Tag<"RectHierarchyUpdate"_hs>>([&](auto, auto&, auto& info, auto&) {
		// FIXME: use a min-heap for optimal sorting
		updates.emplace_back(static_cast<size_t>(&info - rectInfoData));
	});
	int n = 0;
	int minj = INT_MAX;
	int maxj = 0;
	g_ecs->clear_pool<ECS::Tag<"RectHierarchyUpdate"_hs>>();
	for (size_t j : updates) {
		while (j > 0 && rectInfoData[j] < rectInfoData[j - 1]) {
			if (j < minj)
				minj = j;
			group.swap(j, j - 1);
			--j;
			n++;
		}
		if (j > 0 && j > maxj)
			maxj = j;
	}
	m_rectUpdateRanges.add({ static_cast<uint32_t>(minj), static_cast<uint32_t>(maxj) });
	//printf("number of rects: %d\n", g_ecs->get_pool<Game::RectInstance>().size());
	//printf("number of swaps: %d\n", n);
}

