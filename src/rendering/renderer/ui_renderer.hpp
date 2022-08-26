#pragma once

#include <core/local.hpp>

#include <ecs/ecs_fwd.hpp>

#include <rendering/render_pipeline.hpp>
#include <rendering/image.hpp>
#include <rendering/image_view.hpp>
#include <rendering/buffer.hpp>
#include <rendering/sampler.hpp>
#include <rendering/render_context.hpp>
#include <rendering/range_builder.hpp>

#include <ui/ui_render_components.hpp>

// Non-clipped rects: indexed into a single instance buffer of rect data, selected by instance
// begin (mat3x2 transform, vec4(vec2 texTopLeft, vec2 texSize), vec4 color)
// Pipeline: Single texture sampled, xyz mixed with color, w used as blend factor, baked triangle
// vertex buffer, bound instance buffer
//
// Clipped rects: varies at 2-6 tris, might just need a big vertex buffer:
// indexed model - vertex buffer is an unsorted set of triangle data (vec2 pos, vec2 texcoord)
//		- index buffer needs to maintain density and defines draw order
//		- vertex buffer can have gaps, but gaps need to be maintained in a freelist so they can be
//		  recycled
//		- draws are issued vkCmdDrawIndexed(numIndices = 3 * numTris, indexStart = whatever)
//			- UI renderer needs to maintain a list of {num tris, index start} structs defining clip
//			  objects
//			- with this method, is there any benefit to using an index buffer then?
//				- index buffer allows you to swap smaller things to maintain draw order

class TextBitmap;
class CommandBuffer;

class UIRenderer {
	public:
		explicit UIRenderer(TextBitmap&, VkRenderPass, uint32_t subpassIndex);

		NULL_COPY_AND_ASSIGN(UIRenderer);

		constexpr uint32_t get_default_image_index() const {
			return 0u;
		}

		constexpr uint32_t get_text_image_index() const {
			return 1u;
		}

		constexpr uint32_t get_text_sampler() const {
			return 1u;
		}

		uint32_t get_image_index(VkImageView);

		TextBitmap& get_text_bitmap();

		void update(CommandBuffer&);
		void render(CommandBuffer&);

		void mark_rect_for_update(ECS::Entity);
	private:
		TextBitmap& m_textBitmap;

		VkDescriptorSet m_imageDescriptors[RenderContext::FRAMES_IN_FLIGHT];
		std::vector<VkDescriptorImageInfo> m_boundImageInfo;
		std::unordered_map<VkImageView, uint32_t> m_imageIndices;
		size_t m_neededDescriptorUpdates;

		std::shared_ptr<Pipeline> m_pipeline;

		std::shared_ptr<Buffer> m_gpuRectInstances;

		std::shared_ptr<Image> m_blankImage;
		std::shared_ptr<ImageView> m_viewBlankImage;

		std::shared_ptr<Sampler> m_nearestSampler;
		std::shared_ptr<Sampler> m_linearSampler;

		RangeBuilder m_rectUpdateRanges;

		void init_pipelines(VkRenderPass, uint32_t);
		void init_buffers();
		void init_images();
		void init_descriptors();

		void fetch_rect_updates();
};

inline Local<UIRenderer> g_uiRenderer;

