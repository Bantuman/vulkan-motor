#pragma once

#include <volk.h>

#include <functional>
#include <vector>
#include "core/common.hpp"

class RenderContext;
class Image;
class Buffer;

class StagingContext final {
	public:
		//typedef void (*DeletorFunction)(void*);
		using DeletorFunction = std::function<void()>;

		explicit StagingContext(RenderContext&, VkCommandPool, VkFence);

		StagingContext& add_buffer(Buffer&, const void* inputMemory,
				DeletorFunction deletor = nullptr);
		StagingContext& add_buffer(Buffer&, const void* inputMemory, size_t inputSize,
				DeletorFunction deletor = nullptr);

		StagingContext& add_image(Image&, const void* inputMemory, size_t inputSize,
				const VkImageSubresourceRange& range, VkImageLayout finalLayout,
				VkAccessFlagBits finalAccess, DeletorFunction deletor = nullptr);

		void submit();

		NULL_COPY_AND_ASSIGN(StagingContext);
	private:
		enum StagingType {
			STAGING_TYPE_BUFFER,
			STAGING_TYPE_IMAGE
		};

		struct StagingInfo {
			const void* inputMemory;
			size_t inputSize;
			StagingType type;
			DeletorFunction deletor;
			union {
				struct {
					VkBuffer buffer;
				} buffer;
				struct {
					VkImage image;
					VkExtent3D extent;
					VkImageSubresourceRange range;
					VkImageLayout finalLayout;
					VkAccessFlagBits finalAccess;
				} image;
			};
		};

		std::vector<StagingInfo> stagedObjects;
		RenderContext* context;
		VkCommandPool commandPool;
		VkFence fence;
};

