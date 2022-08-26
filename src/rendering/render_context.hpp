#pragma once

#include <core/common.hpp>
#include <core/local.hpp>
#include <core/events.hpp>

#include <rendering/image.hpp>
#include <rendering/image_view.hpp>
#include <rendering/framebuffer.hpp>
#include <rendering/buffer.hpp>
#include <rendering/sampler.hpp>
#include <rendering/descriptors.hpp>
#include <rendering/pipeline_layout.hpp>
#include <rendering/staging_context.hpp>
#include <rendering/command_buffer.hpp>

#include <vector>
#include <functional>
#include <memory>
#include <optional>

class Window;
class RenderContext;

class DeletionQueue {
	public:
		template <typename Deletor>
		void push_back(Deletor&& deletor) {
			static_assert(sizeof(Deletor) <= 200, "Warning: overallocating deletors");
			deletors.push_back(std::move(deletor));
		}

		bool is_empty() const;

		void flush();
	private:
		std::vector<std::function<void()>> deletors;
};

class RenderContext final {
	public:
		using ResizeEvent = Event::Dispatcher<int, int>;

		static constexpr const size_t FRAMES_IN_FLIGHT = 2;

		explicit RenderContext();
		~RenderContext();

		void frame_begin();
		void frame_end();

		template <typename Functor>
		void immediate_submit(Functor&& functor) {
			auto cmd = immediate_submit_begin_internal();
			functor(cmd);
			immediate_submit_end_internal(cmd);
		}

		[[nodiscard]] std::shared_ptr<Buffer> buffer_create(VkDeviceSize size, VkBufferUsageFlags,
				VmaMemoryUsage, VkMemoryPropertyFlags requiredFlags = 0);
		[[nodiscard]] std::shared_ptr<Image> image_create(const VkImageCreateInfo&,
				VmaMemoryUsage, VkMemoryPropertyFlags requiredFlags = 0);
		[[nodiscard]] std::shared_ptr<ImageView> image_view_create(std::shared_ptr<Image> image,
				VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectFlags,
				uint32_t mipLevels = 1, uint32_t arrayLayers = 1);
		[[nodiscard]] std::shared_ptr<ImageView> image_view_create(const VkImageViewCreateInfo&);
		[[nodiscard]] std::shared_ptr<Sampler> sampler_create(const VkSamplerCreateInfo&);
		[[nodiscard]] std::shared_ptr<Framebuffer> framebuffer_create(VkRenderPass,
				uint32_t attachmentCount, const VkImageView* pAttachments, uint32_t width,
				uint32_t height);

		[[nodiscard]] VkDescriptorSetLayout descriptor_set_layout_create(
				const VkDescriptorSetLayoutCreateInfo&);
		[[nodiscard]] VkPipelineLayout pipeline_layout_create(
				const VkPipelineLayoutCreateInfo&);

		[[nodiscard]] std::shared_ptr<StagingContext> staging_context_create();

		DescriptorBuilder global_descriptor_set_begin();
		DescriptorBuilder dynamic_descriptor_set_begin();

		template <typename Deletor>
		void queue_delete(Deletor&& deletor) {
			frames[get_frame_index()].deletionQueue.push_back(std::move(deletor));
		}

		template <typename Deletor>
		void queue_delete_late(Deletor&& deletor) {
			frames[get_last_frame_index()].deletionQueue.push_back(std::move(deletor));
		}

		void swap_chain_recreate(int width, int height);

		size_t pad_uniform_buffer_size(size_t originalSize) const;

		VkInstance get_instance();
		VkPhysicalDevice get_physical_device();
		VkDevice get_device();
		VmaAllocator get_allocator();

		VkQueue get_transfer_queue();
		uint32_t get_transfer_queue_family() const;

		VkQueue get_graphics_queue();
		uint32_t get_graphics_queue_family() const;

		VkCommandBuffer get_graphics_upload_command_buffer();

		VkFormat get_swapchain_image_format() const;
		VkExtent2D get_swapchain_extent() const;
		VkExtent3D get_swapchain_extent_3d() const;

		std::shared_ptr<CommandBuffer> get_main_command_buffer() const;

		size_t get_frame_number() const;
		size_t get_frame_index() const;
		size_t get_last_frame_index() const;

		VkImage get_swapchain_image() const;
		VkImageView get_swapchain_image_view(uint32_t index) const;
		uint32_t get_swapchain_image_index() const;
		uint32_t get_swapchain_image_count() const;

		ResizeEvent& swapchain_resize_event();

		NULL_COPY_AND_ASSIGN(RenderContext);
	private:
		struct FrameData {
			VkFence renderFence;
			VkSemaphore presentSemaphore;
			VkSemaphore renderSemaphore;

			std::shared_ptr<CommandBuffer> mainCommandBuffer;
			Local<DescriptorAllocator> descriptorAllocator;

			uint32_t imageIndex;

			DeletionQueue deletionQueue;
		};

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkPhysicalDevice physicalDevice;
		VkDevice device;

		VkSurfaceKHR surface;

		VkQueue graphicsQueue;
		VkQueue presentQueue;
		VkQueue transferQueue;

		uint32_t graphicsQueueFamily;
		uint32_t transferQueueFamily;

		VkSwapchainKHR swapchain;
		VkFormat swapchainImageFormat;
		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;
		bool invalidSwapchain;

		VmaAllocator allocator;

		VkCommandPool commandPool;
		Local<DescriptorLayoutCache> descriptorLayoutCache;
		Local<DescriptorAllocator> globalDescriptorAllocator;
		Local<PipelineLayoutCache> pipelineLayoutCache;

		VkCommandPool uploadCommandPool;
		VkFence uploadFence;
		VkCommandBuffer graphicsUploadCommandBuffer;

		FrameData frames[FRAMES_IN_FLIGHT];

		size_t frameCounter;

		Window& window;
		ResizeEvent resizeEvent;

		DeletionQueue mainDeletionQueue;

		VkPhysicalDeviceProperties gpuProperties;

		VkCommandBuffer immediate_submit_begin_internal();

		void immediate_submit_end_internal(VkCommandBuffer);

		void vulkan_init();
		void allocator_init();
		void swapchain_init();
		void command_pools_init();
		void frame_data_init();
		void descriptors_init();
		void staging_context_init();
};

inline Local<RenderContext> g_renderContext;

