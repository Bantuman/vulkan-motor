#include "geom.hpp"

#include <core/hashed_string.hpp>
#include <core/logging.hpp>

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/geom_instance.hpp>
#include <rendering/renderer/geom_renderer.hpp>

#include <math/color3.hpp>

using namespace Game;

void Geometry::create(ECS::Manager& ecs, ECS::Entity entity) {
	ecs.add_component<Geometry>(entity);
}

void Geometry::on_destroyed(ECS::Manager& ecs, Instance&, ECS::Entity selfEntity) {
	auto& part = ecs.get_component<Geometry>(selfEntity);
	g_partRenderer->remove_instance(part.m_shape, part.m_transparency == 0.f, selfEntity);
}

void Geometry::set_transform(ECS::Entity selfEntity, Math::Transform transform) {
	auto& inst = g_partRenderer->get_or_add_instance(m_shape, m_transparency == 0.f, selfEntity);
	memcpy(&inst.m_transform, &transform, sizeof(Math::Transform));

	m_transform = std::move(transform);
}

void Geometry::set_size(ECS::Entity selfEntity, Math::Vector3 size) {
	m_size = std::move(size);

	auto& inst = g_partRenderer->get_or_add_instance(m_shape, m_transparency == 0.f, selfEntity);
	memcpy(&inst.m_scale, &size, sizeof(Math::Vector3));
}

void Geometry::set_color(ECS::Entity selfEntity, Math::Color3uint8 color) {
	if (m_color.argb == color.argb) {
		return;
	}

	m_color = color;

	auto& inst = g_partRenderer->get_or_add_instance(m_shape, m_transparency == 0.f, selfEntity);
	inst.m_color = Math::Color3(color).to_vector4(1.f - m_transparency);
}

void Geometry::set_transparency(ECS::Entity selfEntity, float transparency) {
	if (m_transparency == transparency) {
		return;
	}

	if (transparency == 1.f) {
		g_partRenderer->remove_instance(m_shape, m_transparency == 0.f, selfEntity);
	}
	else if (transparency == 0.f || m_transparency == 0.f) {
		auto& oldInst = g_partRenderer->get_or_add_instance(m_shape, m_transparency == 0.f,
				selfEntity);
		auto& newInst = g_partRenderer->get_or_add_instance(m_shape, transparency == 0.f,
				selfEntity);

		memcpy(&newInst, &oldInst, sizeof(GeomInstance));

		g_partRenderer->remove_instance(m_shape, m_transparency == 0.f, selfEntity);
	}
	else {
		auto& inst = g_partRenderer->get_or_add_instance(m_shape, m_transparency == 0.f,
				selfEntity);
		inst.m_color = Math::Color3(m_color).to_vector4(1.f - transparency);
	}

	m_transparency = transparency;
}

void Geometry::set_reflectance(ECS::Entity selfEntity, float reflectance) {
	if (m_reflectance == reflectance) {
		return;
	}

	m_reflectance = reflectance;

	auto& inst = g_partRenderer->get_or_add_instance(m_shape, m_transparency == 0.f, selfEntity);
	inst.m_scale.w = reflectance;
}

void Geometry::set_surface_type(ECS::Entity selfEntity, NormalId normalId,
		SurfaceType surfaceType) {
	auto& inst = g_partRenderer->get_or_add_instance(m_shape, m_transparency == 0.f, selfEntity);

	m_surfaceTypes |= static_cast<uint8_t>(surfaceType) << (4 * static_cast<uint8_t>(normalId));
	inst.m_surfaceTypes = m_surfaceTypes;
}

void Geometry::set_shape(ECS::Entity selfEntity, GeomType shape) {
	if (m_shape == shape) {
		return;
	}

	auto& oldInst = g_partRenderer->get_or_add_instance(m_shape, m_transparency == 0.f,
			selfEntity);
	auto& newInst = g_partRenderer->get_or_add_instance(shape, m_transparency == 0.f, selfEntity);

	memcpy(&newInst, &oldInst, sizeof(GeomInstance));

	g_partRenderer->remove_instance(m_shape, m_transparency == 0.f, selfEntity);

	m_shape = shape;
}

const Math::Transform& Geometry::get_transform() const {
	return m_transform;
}

const Math::Vector3& Geometry::get_size() const {
	return m_size;
}

Math::Color3uint8 Geometry::get_color() const {
	return m_color;
}

float Geometry::get_transparency() const {
	return m_transparency;
}

float Geometry::get_reflectance() const {
	return m_reflectance;
}

SurfaceType Geometry::get_surface_type(NormalId face) const {
	return static_cast<SurfaceType>((m_surfaceTypes >> (4 * static_cast<uint8_t>(face))) & 0xF);
}

GeomType Geometry::get_shape() const {
	return m_shape;
}

