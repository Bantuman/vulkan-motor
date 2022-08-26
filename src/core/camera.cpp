#include "camera.hpp"

#include <ecs/ecs.hpp>

using namespace Game;

void Camera::create(ECS::Manager& ecs, ECS::Entity entity) {
	ecs.add_component<Camera>(entity);
}

void Camera::set_transform(Math::Transform transform) {
	m_transform = std::move(transform);
}

void Camera::set_focus(Math::Transform transform) {
	m_focus = std::move(transform);
}

void Camera::set_field_of_view(float fieldOfView) {
	m_fieldOfView = fieldOfView;
}

void Camera::set_near_plane_z(float nearPlaneZ) {
	m_nearPlaneZ = nearPlaneZ;
}

const Math::Transform& Camera::get_transform() const {
	return m_transform;
}

const Math::Transform& Camera::get_focus() const {
	return m_focus;
}

float Camera::get_field_of_view() const {
	return m_fieldOfView;
}

float Camera::get_near_plane_z() const {
	return m_nearPlaneZ;
}

