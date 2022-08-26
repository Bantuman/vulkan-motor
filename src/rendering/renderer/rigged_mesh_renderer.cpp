#include "rigged_mesh_renderer.hpp"

#include <asset/texture_cache.hpp>

#include <animation/rig.hpp>

#include <core/logging.hpp>

#include <ecs/ecs.hpp>

#include <core/mesh_geom_instance.hpp>
#include <animation/rig_component.hpp>

#include <rendering/rigged_mesh.hpp>
#include <rendering/render_pipeline.hpp>
#include <rendering/shader_program.hpp>
#include <rendering/vk_initializers.hpp>

#include <core/instance.hpp>
#include <core/mesh_geom.hpp>

using namespace Game;

static constexpr const size_t INITIAL_CAPACITY = 64;
static constexpr const size_t INITIAL_RIG_BUFFER_CAPACITY = 512;

// RenderKey

bool RenderKey::operator==(const RenderKey& other) const {
	return rig == other.rig && mesh == other.mesh;
}

bool RenderKey::operator!=(const RenderKey& other) const {
	return !(*this == other);
}

bool RenderKey::operator<(const RenderKey& other) const {
	return (rig < other.rig) || (rig == other.rig && mesh < other.mesh);
};

// RiggedMeshRenderer

RiggedMeshRenderer::RiggedMeshRenderer(VkRenderPass renderPass, uint32_t subpassIndex,
			VkSampleCountFlagBits opaqueSamples)
		: m_instances(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		, m_rigInstances(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
		, m_imageDescriptors(32)
		, m_needsRigUpdate(false) {
	auto samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	auto linearSampler = g_renderContext->sampler_create(samplerInfo);

	m_imageDescriptors.set_sampler(std::move(linearSampler));

	m_defaultDiffuseIndex = m_imageDescriptors.get_image_index(
			*g_textureCache->get("DefaultDiffuseMap")->get_image_view());
	m_defaultNormalIndex = m_imageDescriptors.get_image_index(
			*g_textureCache->get("DefaultNormalMap")->get_image_view());

	auto riggedMeshShader = ShaderProgramBuilder()
		.add_shader("shaders://toon_rigged_mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
		.add_shader("shaders://toon_rigged_mesh.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, true)
		.build(*g_renderContext);

	auto outlineShader = ShaderProgramBuilder()
		.add_shader("shaders://toon_rigged_mesh_outline.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
		.add_shader("shaders://toon_rigged_mesh_outline.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, true)
		.build(*g_renderContext);

	m_pipeline = PipelineBuilder()
		.set_vertex_attribute_descriptions(RiggedMesh::input_attribute_descriptions())
		.set_vertex_binding_descriptions(RiggedMesh::input_binding_descriptions())
		.set_depth_test_enabled(true)
		.set_depth_write_enabled(true)
		.set_depth_compare_op(VK_COMPARE_OP_GREATER_OR_EQUAL)
		.set_sample_count(opaqueSamples)
		.add_program(*riggedMeshShader)
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.build(renderPass, subpassIndex);

	m_outlinePipeline = PipelineBuilder()
		.set_vertex_attribute_descriptions(RiggedMesh::input_attribute_descriptions())
		.set_vertex_binding_descriptions(RiggedMesh::input_binding_descriptions())
		.set_cull_mode(VK_CULL_MODE_FRONT_BIT)
		.set_depth_test_enabled(true)
		.set_depth_write_enabled(true)
		.set_depth_compare_op(VK_COMPARE_OP_GREATER_OR_EQUAL)
		.set_sample_count(opaqueSamples)
		.add_program(*outlineShader)
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.build(renderPass, subpassIndex);

	g_ecs->component_added_event<RigComponent>().connect([&](auto entity, auto& rc) {
		auto* dstData = get_or_add_rig_instance(*rc.m_rig, entity);
		memcpy(dstData, rc.m_finalBoneTransforms.data(),
				rc.m_rig->get_num_bones() * sizeof(Math::Matrix4x4));
	});

	g_ecs->component_removed_event<RigComponent>().connect([&](auto, auto&) {
		m_needsRigUpdate = true;
	});
}

void RiggedMeshRenderer::update() {
	m_imageDescriptors.update();

	if (m_needsRigUpdate) {
		m_needsRigUpdate = false;

		// FIXME: Go through all the rigged meshes and update their rig index
	}
}

void RiggedMeshRenderer::render(CommandBuffer& cmd, VkDescriptorSet dset) {
	auto imgSet = m_imageDescriptors.get_descriptor_set();
	render_internal(cmd, dset, imgSet, *m_pipeline, m_instances);
	render_internal(cmd, dset, imgSet, *m_outlinePipeline, m_instances);
}

MeshGeomInstance& RiggedMeshRenderer::get_or_add_instance(Memory::SharedPtr<RiggedMesh> mesh,
		bool opaque, ECS::Entity entity) {
	RenderKey key{mesh->get_rig(), std::move(mesh)};

	if (opaque) {
		return *reinterpret_cast<MeshGeomInstance*>(m_instances.get_or_add_instance(key,
				sizeof(MeshGeomInstance), entity));
	}
	else {
		return *reinterpret_cast<MeshGeomInstance*>(m_instances.get_or_add_instance(key,
				sizeof(MeshGeomInstance), entity));
	}
}

void RiggedMeshRenderer::remove_instance(Memory::SharedPtr<RiggedMesh> mesh,
	bool opaque, ECS::Entity entity) {
	RenderKey key{mesh->get_rig(), std::move(mesh)};

	if (opaque) {
		m_instances.remove_instance(key, entity);
	}
	else {
		m_instances.remove_instance(key, entity);
	}
}

Math::Matrix4x4* RiggedMeshRenderer::get_or_add_rig_instance(const Rig& rig, ECS::Entity entity) {
	return reinterpret_cast<Math::Matrix4x4*>(m_rigInstances.get_or_add_instance(rig.get_rig_id(),
			rig.get_num_bones() * sizeof(Math::Matrix4x4), entity));
}

void RiggedMeshRenderer::remove_rig_instance(const Rig& rig, ECS::Entity entity) {
	m_rigInstances.remove_instance(rig.get_rig_id(), entity);
}

uint32_t RiggedMeshRenderer::get_image_index(VkImageView imageView) {
	return m_imageDescriptors.get_image_index(imageView);
}

InstanceBucketCollection<RenderKey>& RiggedMeshRenderer::get_instances()
{
	return m_instances;
}

uint32_t RiggedMeshRenderer::get_default_diffuse_index() const {
	return m_defaultDiffuseIndex;
}

uint32_t RiggedMeshRenderer::get_default_normal_index() const {
	return m_defaultNormalIndex;
}

void RiggedMeshRenderer::render_internal(CommandBuffer& cmd, VkDescriptorSet dset,
		VkDescriptorSet imgSet, Pipeline& pipeline,
		InstanceBucketCollection<RenderKey>& instances) {
	cmd.bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_layout(),
			0, 1, &dset, 0, nullptr);
	cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_layout(),
			2, 1, &imgSet, 0, nullptr);

	Rig* currentRig = nullptr;
	for (auto& [key, instData] : instances) {
		if (instData.empty()) {
			continue;
		}
		if (key.rig.get() != currentRig) {
			currentRig = key.rig.get();

			auto& rigInstBucket = m_rigInstances.get_bucket(key.rig->get_rig_id());
			VkDescriptorBufferInfo boneBuf{};
			boneBuf.buffer = rigInstBucket.get_buffer();
			boneBuf.offset = 0;
			boneBuf.range = rigInstBucket.size() * key.rig->get_num_bones()
					* sizeof(Math::Matrix4x4);

			auto boneSet = g_renderContext->dynamic_descriptor_set_begin()
					.bind_buffer(0, boneBuf, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
							VK_SHADER_STAGE_VERTEX_BIT)
					.build();

			cmd.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_layout(),
					1, 1, &boneSet, 0, nullptr);
		}

		VkBuffer buffers[] = {*key.mesh->get_vertex_buffer(), instData.get_buffer()};
		VkDeviceSize offsets[2] = {};

		cmd.bind_vertex_buffers(0, 2, buffers, offsets);
		cmd.bind_index_buffer(*key.mesh->get_index_buffer(), 0, VK_INDEX_TYPE_UINT32);

		cmd.draw_indexed(key.mesh->get_num_indices(), instData.size(), 0, 0, 0);
	}
}

