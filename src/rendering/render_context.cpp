#include "render_context.hpp"

#include <vkbootstrap/VkBootstrap.h>

#include <core/application.hpp>

#include <rendering/vk_common.hpp>
#include <rendering/vk_initializers.hpp>
#include <rendering/vk_profiler.hpp>

// DELETION QUEUE

bool DeletionQueue::is_empty() const {
	return deletors.empty();
}

void DeletionQueue::flush() {
	for (auto it = deletors.rbegin(), end = deletors.rend(); it != end; ++it) {
		(*it)();
	}

	deletors.clear();
}

// CONSTRUCTORS/DESTRUCTORS

RenderContext::RenderContext()
		: invalidSwapchain(false)
		//, frames{}
		, frameCounter{}
		, window(*g_window) {
	vulkan_init();
	allocator_init();
	swapchain_init();
	command_pools_init();
	frame_data_init();
	descriptors_init();
	staging_context_init();
}

RenderContext::~RenderContext() {
	vkDeviceWaitIdle(device);

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		auto& frame = frames[i];
		frame.deletionQueue.flush();
	}

	mainDeletionQueue.flush();

	// flush frames again for stuff put in by the main deletion queue
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		auto& frame = frames[i];
		frame.deletionQueue.flush();
	}

	for (auto& frame : frames) {
		vkDestroySemaphore(device, frame.renderSemaphore, nullptr);
		vkDestroySemaphore(device, frame.presentSemaphore, nullptr);
		vkDestroyFence(device, frame.renderFence, nullptr);

		frame.descriptorAllocator.destroy();
	}

	for (auto imageView : swapchainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);

	vmaDestroyAllocator(allocator);

	g_vulkanProfiler.destroy();

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(device, nullptr);
	vkb::destroy_debug_utils_messenger(instance, debugMessenger);
	vkDestroyInstance(instance, nullptr);
}

// PUBLIC METHODS

void RenderContext::frame_begin() {
	auto& frame = frames[get_frame_index()];

	VK_CHECK(vkWaitForFences(device, 1, &frame.renderFence, VK_TRUE, UINT64_MAX));

	frame.deletionQueue.flush();
	frame.descriptorAllocator->reset_pools();

	frame.mainCommandBuffer->reset(0);

	auto result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, frame.presentSemaphore,
			VK_NULL_HANDLE, &frame.imageIndex);
	invalidSwapchain = result != VK_SUCCESS;

	if (!invalidSwapchain) {
		VK_CHECK(vkResetFences(device, 1, &frame.renderFence));
	}

	frame.mainCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	g_vulkanProfiler->grab_queries(frame.mainCommandBuffer);
}

void RenderContext::frame_end() {
	auto& frame = frames[get_frame_index()];

	frame.mainCommandBuffer->end();

	if (!invalidSwapchain) {
		auto cmd = frame.mainCommandBuffer->get_buffer();
		VkSubmitInfo submitInfo = vkinit::submit_info(cmd);

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitInfo.pWaitDstStageMask = &waitStage;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &frame.presentSemaphore;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &frame.renderSemaphore;

		VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.renderFence));

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &frame.renderSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &frame.imageIndex;

		vkQueuePresentKHR(presentQueue, &presentInfo);
	}

	++frameCounter;
}

std::shared_ptr<Buffer> RenderContext::buffer_create(VkDeviceSize size,
		VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage,
		VkMemoryPropertyFlags requiredFlags) {
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usageFlags;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsage;
	allocInfo.requiredFlags = requiredFlags;

	VkBuffer buffer;
	VmaAllocation allocation;

	if (vmaCreateBuffer(allocator, &createInfo, &allocInfo, &buffer, &allocation,
			nullptr) == VK_SUCCESS) {
		return std::make_shared<Buffer>(*this, buffer, allocation, size);
	}

	return nullptr;
}

std::shared_ptr<Image> RenderContext::image_create(const VkImageCreateInfo& createInfo,
		VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags) {
	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsage;
	allocInfo.requiredFlags = requiredFlags;

	VkImage image;
	VmaAllocation allocation;

	if (vmaCreateImage(allocator, &createInfo, &allocInfo, &image, &allocation, nullptr)
			== VK_SUCCESS) {
		return std::make_shared<Image>(image, allocation, createInfo.extent, createInfo.format,
				createInfo.samples);
	}

	return nullptr;
}

std::shared_ptr<ImageView> RenderContext::image_view_create(std::shared_ptr<Image> image,
		VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectFlags,
		uint32_t mipLevels, uint32_t arrayLayers) {
	VkImageViewCreateInfo createInfo = vkinit::image_view_create_info(viewType, format,
			image->get_image(), aspectFlags, mipLevels, arrayLayers);

	return image_view_create(createInfo);
}

std::shared_ptr<ImageView> RenderContext::image_view_create(
		const VkImageViewCreateInfo& createInfo) {
	VkImageView imageView;
	if (vkCreateImageView(device, &createInfo, nullptr, &imageView) == VK_SUCCESS) {
		return std::make_shared<ImageView>(*this, imageView);
	}

	return nullptr;
}

std::shared_ptr<Sampler> RenderContext::sampler_create(const VkSamplerCreateInfo& createInfo) {
	VkSampler sampler;
	if (vkCreateSampler(device, &createInfo, nullptr, &sampler) == VK_SUCCESS) {
		return std::make_shared<Sampler>(*this, sampler);
	}

	return nullptr;
}

std::shared_ptr<Framebuffer> RenderContext::framebuffer_create(VkRenderPass renderPass,
		uint32_t attachmentCount, const VkImageView* pAttachments, uint32_t width,
		uint32_t height) {
	VkFramebufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.renderPass = renderPass;
	createInfo.width = width;
	createInfo.height = height;
	createInfo.layers = 1;
	createInfo.attachmentCount = attachmentCount;
	createInfo.pAttachments = pAttachments;

	VkFramebuffer framebuffer;
	if (vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffer) == VK_SUCCESS) {
		return std::make_shared<Framebuffer>(*this, framebuffer);
	}

	return nullptr;
}

VkDescriptorSetLayout RenderContext::descriptor_set_layout_create(
		const VkDescriptorSetLayoutCreateInfo& createInfo) {
	return descriptorLayoutCache->get(createInfo);
}

VkPipelineLayout RenderContext::pipeline_layout_create(
		const VkPipelineLayoutCreateInfo& createInfo) {
	return pipelineLayoutCache->get(createInfo);
}

std::shared_ptr<StagingContext> RenderContext::staging_context_create() {
	return std::make_shared<StagingContext>(*this, uploadCommandPool, uploadFence);
}

DescriptorBuilder RenderContext::global_descriptor_set_begin() {
	return DescriptorBuilder(*descriptorLayoutCache, *globalDescriptorAllocator);
}

DescriptorBuilder RenderContext::dynamic_descriptor_set_begin() {
	return DescriptorBuilder(*descriptorLayoutCache,
			*frames[get_frame_index()].descriptorAllocator);
}

size_t RenderContext::pad_uniform_buffer_size(size_t originalSize) const {
	size_t minUboAlignment = gpuProperties.limits.minUniformBufferOffsetAlignment;

	if (minUboAlignment == 0) {
		return originalSize;
	}

	return (originalSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
}

VkInstance RenderContext::get_instance() {
	return instance;
}

VkPhysicalDevice RenderContext::get_physical_device() {
	return physicalDevice;
}

VkDevice RenderContext::get_device() {
	return device;
}

VmaAllocator RenderContext::get_allocator() {
	return allocator;
}

VkQueue RenderContext::get_transfer_queue() {
	return transferQueue;
}

uint32_t RenderContext::get_transfer_queue_family() const {
	return transferQueueFamily;
}

VkQueue RenderContext::get_graphics_queue() {
	return graphicsQueue;
}

uint32_t RenderContext::get_graphics_queue_family() const {
	return graphicsQueueFamily;
}

VkCommandBuffer RenderContext::get_graphics_upload_command_buffer() {
	return graphicsUploadCommandBuffer;
}

VkFormat RenderContext::get_swapchain_image_format() const {
	return swapchainImageFormat;
}

VkExtent2D RenderContext::get_swapchain_extent() const {
	return {static_cast<uint32_t>(window.get_width()),
			static_cast<uint32_t>(window.get_height())};
}

VkExtent3D RenderContext::get_swapchain_extent_3d() const {
	return {static_cast<uint32_t>(window.get_width()),
			static_cast<uint32_t>(window.get_height()), 1};
}

std::shared_ptr<CommandBuffer> RenderContext::get_main_command_buffer() const {
	return frames[get_frame_index()].mainCommandBuffer;
}

size_t RenderContext::get_frame_number() const {
	return frameCounter;
}

size_t RenderContext::get_frame_index() const {
	if constexpr (FRAMES_IN_FLIGHT == 2) {
		return (frameCounter & 1);
	}
	else {
		return frameCounter % FRAMES_IN_FLIGHT;
	}
}

size_t RenderContext::get_last_frame_index() const {
	if constexpr (FRAMES_IN_FLIGHT == 2) {
		return (frameCounter - 1) & 1;
	}
	else {
		return (frameCounter - 1) % FRAMES_IN_FLIGHT;
	}
}

VkImage RenderContext::get_swapchain_image() const {
	return swapchainImages[get_swapchain_image_index()];
}

VkImageView RenderContext::get_swapchain_image_view(uint32_t index) const {
	return swapchainImageViews[index];
}

uint32_t RenderContext::get_swapchain_image_index() const {
	return frames[get_frame_index()].imageIndex;
}

uint32_t RenderContext::get_swapchain_image_count() const {
	return static_cast<uint32_t>(swapchainImages.size());
}

RenderContext::ResizeEvent& RenderContext::swapchain_resize_event() {
	return resizeEvent;
}

// PRIVATE METHODS

VkCommandBuffer RenderContext::immediate_submit_begin_internal() {
	VkCommandBuffer cmd;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &cmd));

	VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

	return cmd;
}

void RenderContext::immediate_submit_end_internal(VkCommandBuffer cmd) {
	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submitInfo = vkinit::submit_info(cmd);
	// FIXME: maybe don't tie up the upload fence
	VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, uploadFence));

	vkWaitForFences(device, 1, &uploadFence, true, UINT64_MAX);
	vkResetFences(device, 1, &uploadFence);

	vkFreeCommandBuffers(device, commandPool, 1, &cmd);
}

// INIT

void RenderContext::vulkan_init() {
	VK_CHECK(volkInitialize());

	vkb::InstanceBuilder instanceBuilder;
	auto vkbInstance = instanceBuilder
			.set_app_name("My Vulkan Engine")
			.request_validation_layers(true)
			.require_api_version(1, 2, 0)
			.use_default_debug_messenger()
			.build()
			.value();
	
	instance = vkbInstance.instance;
	debugMessenger = vkbInstance.debug_messenger;

	volkLoadInstance(instance);

	VK_CHECK(glfwCreateWindowSurface(instance, window.get_handle(), nullptr, &surface));

	VkPhysicalDeviceFeatures features{};
	features.fragmentStoresAndAtomics = true;
	features.pipelineStatisticsQuery = true;

	vkb::PhysicalDeviceSelector selector{vkbInstance};
	auto vkbPhysicalDevice = selector
			.set_minimum_version(1, 1)
			.set_surface(surface)
			.set_required_features(features)
			.add_required_extension("VK_EXT_descriptor_indexing")
			.select()
			.value();

	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexing{};
	descriptorIndexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	descriptorIndexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexing.runtimeDescriptorArray = VK_TRUE;
	descriptorIndexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
	descriptorIndexing.descriptorBindingPartiallyBound = VK_TRUE;

	vkb::DeviceBuilder deviceBuilder{vkbPhysicalDevice};
	auto vkbDevice = deviceBuilder
			.add_pNext(&descriptorIndexing)
			.build()
			.value();

	if (vkbPhysicalDevice.has_dedicated_compute_queue()) {
		LOG_TEMP2("Device has a dedicated compute queue");
	}

	if (vkbPhysicalDevice.has_separate_compute_queue()) {
		LOG_TEMP2("Device has a separate compute queue");
	}

	device = vkbDevice.device;
	physicalDevice = vkbPhysicalDevice.physical_device;

	gpuProperties = vkbDevice.physical_device.properties;

	volkLoadDevice(device);

	graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();

	if (vkbPhysicalDevice.has_dedicated_transfer_queue()) {
		transferQueue = vkbDevice.get_dedicated_queue(vkb::QueueType::transfer).value();
		transferQueueFamily = vkbDevice.get_dedicated_queue_index(vkb::QueueType::transfer)
				.value();
	}
	else {
		transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
		transferQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();
	}

	g_vulkanProfiler.create(device, gpuProperties.limits.timestampPeriod);
}

void RenderContext::allocator_init() {
	VmaVulkanFunctions vkFns{};
	vkFns.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vkFns.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	vkFns.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
	vkFns.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vkFns.vkAllocateMemory = vkAllocateMemory;
	vkFns.vkFreeMemory = vkFreeMemory;
	vkFns.vkMapMemory = vkMapMemory;
	vkFns.vkUnmapMemory = vkUnmapMemory;
	vkFns.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	vkFns.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	vkFns.vkBindBufferMemory = vkBindBufferMemory;
	vkFns.vkBindImageMemory = vkBindImageMemory;
	vkFns.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
	vkFns.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
	vkFns.vkCreateBuffer = vkCreateBuffer;
	vkFns.vkDestroyBuffer = vkDestroyBuffer;
	vkFns.vkCreateImage = vkCreateImage;
	vkFns.vkDestroyImage = vkDestroyImage;
	vkFns.vkCmdCopyBuffer = vkCmdCopyBuffer;

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;
	allocatorInfo.pVulkanFunctions = &vkFns;
	VK_CHECK(vmaCreateAllocator(&allocatorInfo, &allocator));
}

void RenderContext::swapchain_init() {
	vkb::SwapchainBuilder builder{physicalDevice, device, surface};
	auto vkbSwapchain = builder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // change to mailbox later
		//.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
		.set_desired_extent(window.get_width(), window.get_height())
		.build()
		.value();

	swapchain = vkbSwapchain.swapchain;
	swapchainImageFormat = vkbSwapchain.image_format;
	swapchainImages = vkbSwapchain.get_images().value();
	swapchainImageViews = vkbSwapchain.get_image_views().value();

	window.resize_event().connect([&](int width, int height) {
		swap_chain_recreate(width, height);
	});
}

void RenderContext::command_pools_init() {
	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = graphicsQueueFamily;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	
	VK_CHECK(vkCreateCommandPool(device, &createInfo, nullptr, &commandPool));

	mainDeletionQueue.push_back([=] {
		vkDestroyCommandPool(device, commandPool, nullptr);
	});
}

void RenderContext::frame_data_init() {
	VkCommandBuffer mainCommandBuffers[FRAMES_IN_FLIGHT];

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);

	VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, mainCommandBuffers));

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		auto& frameData = frames[i];

		frameData.mainCommandBuffer = std::make_shared<CommandBuffer>(mainCommandBuffers[i]);
		frameData.descriptorAllocator.create(device);

		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frameData.renderFence));

		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
				&frameData.presentSemaphore));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
				&frameData.renderSemaphore));
	}
}

void RenderContext::descriptors_init() {
	pipelineLayoutCache.create(device);
	descriptorLayoutCache.create(device);
	globalDescriptorAllocator.create(device);

	mainDeletionQueue.push_back([=] {
		globalDescriptorAllocator.destroy();
		descriptorLayoutCache.destroy();
		pipelineLayoutCache.destroy();
	});
}

void RenderContext::staging_context_init() {
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = transferQueueFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	
	VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &uploadCommandPool));

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &graphicsUploadCommandBuffer));

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &uploadFence));

	mainDeletionQueue.push_back([=] {
		vkDestroyFence(device, uploadFence, nullptr);
		vkDestroyCommandPool(device, uploadCommandPool, nullptr);
	});
}

void RenderContext::swap_chain_recreate(int width, int height) {
	if (width == 0 || height == 0) {
		return;
	}
	vkb::SwapchainBuilder builder{physicalDevice, device, surface};
	auto vkbSwapchain = builder
			.set_old_swapchain(swapchain)
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // change to mailbox later
			.set_desired_extent(width, height)
			.build()
			.value();

	queue_delete_late([deviceIn = device, swapchainIn = swapchain,
			viewsToDeleteIn = swapchainImageViews] {
		for (auto imageView : viewsToDeleteIn) {
			vkDestroyImageView(deviceIn, imageView, nullptr);
		}
		vkDestroySwapchainKHR(deviceIn, swapchainIn, nullptr);
	});

	swapchain = vkbSwapchain.swapchain;
	swapchainImages = vkbSwapchain.get_images().value();
	swapchainImageViews = vkbSwapchain.get_image_views().value();

	resizeEvent.fire(width, height);
}

