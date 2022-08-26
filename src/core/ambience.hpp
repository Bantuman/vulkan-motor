#pragma once

#include <string>
#include <string_view>

#include <ecs/ecs_fwd.hpp>

#include <math/color3.hpp>
#include <math/vector3.hpp>

namespace Game {

class Sky;

class Ambience final {
	public:
		static void create(ECS::Manager&, ECS::Entity);

		void set_ambient(Math::Color3);
		void set_outdoor_ambient(Math::Color3);
		void set_brightness(float);

		void set_color_shift_top(Math::Color3);
		void set_color_shift_bottom(Math::Color3);

		void set_fog_color(Math::Color3);
		void set_fog_start(float);
		void set_fog_end(float);

		void set_time_of_day(const std::string_view& dayString);
		void set_geographic_latitude(float);

		void set_active_skybox(ECS::Entity, const Sky*);

		std::string get_time_of_day() const;
		float get_brightness() const;

		Math::Vector3 get_sun_direction() const;

		ECS::Entity get_active_skybox() const;
	private:
		Math::Color3 m_ambient;
		Math::Color3 m_outdoorAmbient;
		float m_brightness;

		// The hue represented in light reflected in the opposite surfaces to those facing the sun
		// or the moon
		Math::Color3 m_colorShiftBottom;
		// The hue represented in light reflected from surfaces facing the sun or the moon
		Math::Color3 m_colorShiftTop;

		Math::Color3 m_fogColor;
		float m_fogStart;
		float m_fogEnd;

		double m_minutesAfterMidnight;
		float m_geographicLatitude;

		// PBR-IBL values
		float m_environmentDiffuseScale;
		float m_environmentSpecularScale;

		ECS::Entity m_activeSkybox = ECS::INVALID_ENTITY;
};

}

