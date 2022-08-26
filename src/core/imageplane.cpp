#include "imageplane.hpp"

#include <asset/texture_cache.hpp>

#include <ecs/ecs.hpp>

#include <core/imageplane_instance.hpp>
#include <rendering/renderer/imageplane_renderer.hpp>
#include <core/instance.hpp>
#include <core/geom.hpp>

using namespace Game;

void ImagePlane::create(ECS::Manager& ecs, ECS::Entity entity) {
	ecs.add_component<ImagePlane>(entity);
}

void ImagePlane::on_ancestry_changed(ECS::Manager& ecs, Instance& selfInst, ECS::Entity selfEntity,
		Instance*, ECS::Entity) {
	// FIXME: newParentInst->is_descendant_of(workspace)
	
	auto& decal = ecs.get_component<ImagePlane>(selfEntity);

	decal.m_renderable = false;
	g_decalRenderer->remove_instance(decal.m_shape, decal.m_face, selfEntity);

	if (selfInst.m_parent != ECS::INVALID_ENTITY) {
		auto& parentInst = ecs.get_component<Instance>(selfInst.m_parent);
		
		if (parentInst.is_a(InstanceClass::BASE_GEOM)
				&& parentInst.m_classID != InstanceClass::MESH_GEOM) {
			auto& geom = ecs.get_component<Geometry>(selfInst.m_parent);
			decal.m_shape = geom.get_shape();
			decal.m_renderable = true;

			decal.full_redraw(geom, selfEntity);
		}
	}
}

void ImagePlane::set_texture(ECS::Manager& ecs, Instance& selfInst, ECS::Entity selfEntity,
		std::string texture) {
	if (m_texture.compare(texture) == 0) {
		return;
	}

	m_texture = std::move(texture);

	if (!is_visible()) {
		return;
	}

	auto& geom = ecs.get_component<Geometry>(selfInst.m_parent);
	full_redraw(geom, selfEntity);
}

void ImagePlane::set_color(ECS::Entity selfEntity, Math::Color3 color) {
	if (m_color == color) {
		return;
	}

	m_color = std::move(color);

	if (!is_visible()) {
		return;
	}

	auto& inst = g_decalRenderer->get_or_add_instance(m_shape, m_face, selfEntity);
	inst.m_color = m_color.to_vector4(1.f - m_transparency);
}

void ImagePlane::set_transparency(ECS::Manager& ecs, Instance& selfInst, ECS::Entity selfEntity,
		float transparency) {
	if (m_transparency == transparency) {
		return;
	}

	auto oldTransparency = m_transparency;
	m_transparency = transparency;

	if (!is_visible()) {
		return;
	}

	if (oldTransparency == 1.f) {
		auto& geom = ecs.get_component<Geometry>(selfInst.m_parent);
		full_redraw(geom, selfEntity);
	}
	else {
		auto& inst = g_decalRenderer->get_or_add_instance(m_shape, m_face, selfEntity);
		inst.m_color = m_color.to_vector4(1.f - m_transparency);
	}
}

void ImagePlane::set_z_index(ECS::Entity selfEntity, int32_t zIndex) {
	auto uZIndex = static_cast<uint32_t>(zIndex) + 0x80'00'00'00u;

	if (uZIndex == m_zIndex) {
		return;
	}

	m_zIndex = uZIndex;
	(void)selfEntity;
}

void ImagePlane::set_face(ECS::Manager& ecs, Instance& selfInst, ECS::Entity selfEntity,
		NormalId face) {
	if (m_face == face) {
		return;
	}

	auto oldFace = m_face;
	m_face = face;

	if (!is_visible()) {
		return;
	}

	g_decalRenderer->remove_instance(m_shape, oldFace, selfEntity);

	auto& geom = ecs.get_component<Geometry>(selfInst.m_parent);

	auto tex = g_textureCache->get_or_load<TextureLoader>(m_texture, *g_renderContext,
			m_texture, true, true);

	if (tex) {
		auto& inst = g_decalRenderer->get_or_add_instance(geom.get_shape(), m_face,
				selfEntity);
		inst.m_transform = geom.get_transform();
		inst.m_scale = geom.get_size();
		inst.m_color = m_color.to_vector4(1.f - m_transparency);
		inst.m_imageIndex = g_decalRenderer->get_image_index(*tex->get_image_view());
	}
	else {
		m_validTexture = false;
	}
}

bool ImagePlane::is_visible() const {
	return m_renderable && m_validTexture && m_transparency != 1.f;
}

void ImagePlane::full_redraw(const Geometry& geom, ECS::Entity selfEntity) {
	auto tex = g_textureCache->get_or_load<TextureLoader>(m_texture, *g_renderContext, m_texture,
			true, true);

	if (tex) {
		m_validTexture = true;

		auto& inst = g_decalRenderer->get_or_add_instance(geom.get_shape(), m_face, selfEntity);
		inst.m_transform = geom.get_transform();
		inst.m_scale = geom.get_size();
		inst.m_color = m_color.to_vector4(1.f - m_transparency);
		inst.m_imageIndex = g_decalRenderer->get_image_index(*tex->get_image_view());
	}
	else {
		m_validTexture = false;
		g_decalRenderer->remove_instance(geom.get_shape(), m_face, selfEntity);
	}
}

