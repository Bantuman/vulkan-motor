#pragma once

#include <ecs/ecs_fwd.hpp>

#include <math/transform.hpp>

namespace Game {

enum class CameraType : uint8_t {
	FIXED = 0,
	WATCH = 2,
	ATTACH = 1,
	TRACK = 3,
	FOLLOW = 4,
	CUSTOM = 5,
	SCRIPTABLE = 6,
	ORBITAL = 7
};

class Camera {
	public:
		static void create(ECS::Manager&, ECS::Entity);

		void set_transform(Math::Transform);
		void set_focus(Math::Transform);
		void set_field_of_view(float);
		void set_near_plane_z(float);

		const Math::Transform& get_transform() const;
		const Math::Transform& get_focus() const;

		float get_field_of_view() const;
		float get_near_plane_z() const;

		float m_speed = 40; // FIXME: temp nonstandard value
	private:
		Math::Transform m_transform;
		Math::Transform m_focus;
		float m_fieldOfView = 70.f;
		float m_nearPlaneZ = 0.1f;
};

}

