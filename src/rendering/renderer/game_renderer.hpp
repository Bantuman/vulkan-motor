#pragma once

#include <core/common.hpp>
#include <core/local.hpp>

#include <rendering/render_context.hpp>
#include <rendering/render_pipeline.hpp>
#include <rendering/render_pass.hpp>
#include <rendering/texture.hpp>
#include <rendering/geom_mesh.hpp>
#include <rendering/cube_map.hpp>
#include <rendering/text_bitmap.hpp>

#include <math/vector2.hpp>
#include <math/vector3.hpp>
#include <math/vector4.hpp>
#include <math/matrix4x4.hpp>

class Window;
class CommandBuffer;

class GameRenderer final {
	public:
		static constexpr const uint32_t DEPTH_MIP_COUNT = 16;

		explicit GameRenderer(RenderContext&);
		~GameRenderer();

		void render();

		void update_camera(const Math::Vector3& position, const Math::Matrix4x4& view);
		void set_sunlight_dir(const Math::Vector3& dir);
		void set_brightness(float brightness);

		void set_skybox(std::shared_ptr<CubeMap> skybox);

		RenderContext& get_context() const;

		TextBitmap& get_text_bitmap();

		NULL_COPY_AND_ASSIGN(GameRenderer);
	private:
		struct FrameData {
			VkDescriptorSet globalDescriptor;
			VkDescriptorSet depthInputDescriptor;
			VkDescriptorSet oitWriteDescriptor;
			VkDescriptorSet oitReadDescriptor;
			VkDescriptorSet colorInputDescriptor;

			bool needsDescriptorUpdate;
		};

		struct SceneData {
			Math::Vector4 fogColor; // w is for exponent
			Math::Vector4 fogDistances; // x = min, y = max, zw unused
			Math::Vector4 ambientColor;
			Math::Vector4 sunlightDirection; // w for sun power
			Math::Vector4 sunlightColor;
		};

		struct CameraData {
			Math::Matrix4x4 view;
			Math::Matrix4x4 projection;
			Math::Vector3 position;
			float padding0;
			Math::Vector4 projInfo;
			Math::Vector2 screenSize;
		};

		struct SSAOData {
			float projScale; // image plane pixels per unit
			float bias; // Bias to avoid AO in smooth corners, e.g. 0.01
			float intensity;
			float radius; // World-space AO radius in scene units
			float radius2; // radius * radius, e.g. 1.0
			float intensityDivR6; // intensity / (radius^6)
		};

		RenderContext* m_context;
		Window* m_window;

		FrameData m_frames[RenderContext::FRAMES_IN_FLIGHT];

		Math::Vector3 m_sunlightDirection;

		CameraData m_cameraData;
		SceneData m_sceneData;
		SSAOData m_ssaoData;

		std::shared_ptr<Buffer> m_sceneDataBuffer;
		std::shared_ptr<Buffer> m_cameraDataBuffer;
		std::shared_ptr<Buffer> m_ssaoDataBuffer;

		uint8_t* m_sceneDataMapping;
		uint8_t* m_cameraDataMapping;
		uint8_t* m_ssaoDataMapping;

		std::shared_ptr<Image> m_depthBufferMS;
		std::shared_ptr<ImageView> m_viewDepthBufferMS;

		std::shared_ptr<Image> m_depthBuffer;
		std::shared_ptr<ImageView> m_viewDepthBuffer;

		std::shared_ptr<Image> m_depthPyramid;
		std::shared_ptr<ImageView> m_viewDepthPyramid;
		std::shared_ptr<ImageView> m_viewDepthMips[DEPTH_MIP_COUNT];

		uint32_t m_depthPyramidWidth;
		uint32_t m_depthPyramidHeight;
		uint32_t m_depthPyramidLevels;

		std::shared_ptr<Image> m_aoImage;
		std::shared_ptr<ImageView> m_viewAOImage;
		std::shared_ptr<Image> m_aoBlurImage;
		std::shared_ptr<ImageView> m_viewAOBlurImage;

		std::shared_ptr<Pipeline> m_depthReducePipeline;

		std::shared_ptr<Pipeline> m_ssaoPipeline;
		std::shared_ptr<Pipeline> m_ssaoBlurPipeline;

		std::shared_ptr<Pipeline> m_oitResolvePipeline;
		std::shared_ptr<Pipeline> m_skyboxPipeline;
		std::shared_ptr<Pipeline> m_toneMapPipeline;

		std::shared_ptr<RenderPass> m_depthPrePass;
		std::shared_ptr<Framebuffer> m_depthPrePassFramebuffer;

		std::shared_ptr<RenderPass> m_aoPass;
		std::shared_ptr<Framebuffer> m_aoFramebuffer;

		std::shared_ptr<RenderPass> m_forwardPass;
		std::shared_ptr<Framebuffer> m_fwdFramebuffer;

		std::shared_ptr<RenderPass> m_outputPass;
		std::vector<std::shared_ptr<Framebuffer>> m_outFramebuffers;

		std::shared_ptr<Image> m_colorAttachment;
		std::shared_ptr<ImageView> m_viewColorAttachment;

		std::shared_ptr<CubeMap> m_defaultSkybox;
		std::shared_ptr<CubeMap> m_skybox;

		std::shared_ptr<Image> m_colorBufferOIT;
		std::shared_ptr<ImageView> m_viewColorBufferOIT;

		std::shared_ptr<Image> m_depthBufferOIT;
		std::shared_ptr<ImageView> m_viewDepthBufferOIT;

		std::shared_ptr<Image> m_visBufferOIT;
		std::shared_ptr<ImageView> m_viewVisBufferOIT;

		std::shared_ptr<Image> m_lockOIT;
		std::shared_ptr<ImageView> m_viewLockOIT;

		std::shared_ptr<Sampler> m_sampler;
		std::shared_ptr<Sampler> m_depthReduceSampler;

		Local<TextBitmap> m_textBitmap;

		VkSampleCountFlagBits m_sampleCount;

		void depth_buffer_init();
		void depth_pre_pass_init();
		void ao_pass_init();
		void forward_pass_init();
		void output_pass_init();
		void renderers_init();
		void buffers_init();
		void frame_data_init();

		void update_buffers();
		void update_instances(CommandBuffer&);
		void update_descriptors();

		void depth_pre_pass(CommandBuffer&);
		void reduce_depth(CommandBuffer&);
		void ao_pass(CommandBuffer&);
		void blur_ao(CommandBuffer&);
		void forward_pass(CommandBuffer&);
		void barrier_oit(CommandBuffer&);
		void output_pass(CommandBuffer&);

		void depth_buffer_images_create(const VkExtent3D&);
		void depth_buffer_recreate(const VkExtent3D&);

		void ao_images_create(const VkExtent3D&);
		void forward_images_create(const VkExtent3D&);
		void oit_images_create(const VkExtent3D&);

		void ao_pass_recreate(const VkExtent3D&);
		void forward_pass_recreate(const VkExtent3D&);
		void oit_pass_recreate(const VkExtent3D&);
		void framebuffers_recreate(uint32_t width, uint32_t height);

		void on_swap_chain_resized(int width, int height);
};

inline Local<GameRenderer> g_renderer;

