#include "ambience.hpp"

#include <cstdio>

#include <asset/cube_map_cache.hpp>

#include <ecs/ecs.hpp>

#include <math/trigonometric.hpp>

#include <rendering/renderer/game_renderer.hpp>
#include <core/sky.hpp>

using namespace Game;

void Ambience::create(ECS::Manager& ecs, ECS::Entity entity) {
	ecs.add_component<Ambience>(entity);
}

void Ambience::set_ambient(Math::Color3 ambient) {
	m_ambient = std::move(ambient);
}

void Ambience::set_outdoor_ambient(Math::Color3 outdoorAmbient) {
	m_outdoorAmbient = std::move(outdoorAmbient);
}

void Ambience::set_brightness(float brightness) {
	m_brightness = brightness;
	g_renderer->set_brightness(m_brightness);
}

void Ambience::set_color_shift_bottom(Math::Color3 colorShiftBottom) {
	m_colorShiftBottom = std::move(colorShiftBottom);
}

void Ambience::set_color_shift_top(Math::Color3 colorShiftTop) {
	m_colorShiftTop = std::move(colorShiftTop);
}

void Ambience::set_time_of_day(const std::string_view& dayString) {
	assert(dayString.size() == 8 && "day string must be in the format hh:mm:ss");
	std::string hourPart(dayString.substr(0, 2));
	std::string minutePart(dayString.substr(3, 5));
	std::string secondPart(dayString.substr(6, 8));

	double hours = std::stod(hourPart);
	double minutes = std::stod(minutePart);
	double seconds = std::stod(secondPart);

	m_minutesAfterMidnight = hours * 60.0 + minutes + seconds / 60.0;

	g_renderer->set_sunlight_dir(get_sun_direction());
}

float Ambience::get_brightness() const
{
	return m_brightness;
}

void Ambience::set_geographic_latitude(float geographicLatitude) {
	m_geographicLatitude = geographicLatitude;

	g_renderer->set_sunlight_dir(get_sun_direction());
}

void Ambience::set_active_skybox(ECS::Entity eSky, const Sky* pSky) {
	if (m_activeSkybox == eSky) {
		return;
	}

	m_activeSkybox = eSky;

	if (pSky) {
		std::string_view skyboxFileNames[] = {
			pSky->get_front_id(),
			pSky->get_back_id(),
			pSky->get_up_id(),
			pSky->get_down_id(),
			pSky->get_right_id(),
			pSky->get_left_id()
		};

		auto cubeMap = g_cubeMapCache->get_or_load<CubeMapLoader>("Skybox", *g_renderContext,
				skyboxFileNames, 6, false);
		g_renderer->set_skybox(std::move(cubeMap));
	}
	else {
		std::string_view skyboxFileNames[] = {
			"res://sky512_ft.dds",
			"res://sky512_bk.dds",
			"res://sky512_up.dds",
			"res://sky512_dn.dds",
			"res://sky512_rt.dds",
			"res://sky512_lf.dds",
		};

		auto cubeMap = g_cubeMapCache->get_or_load<CubeMapLoader>("DefaultSkybox",
				*g_renderContext, skyboxFileNames, 6, false);
		g_renderer->set_skybox(std::move(cubeMap));
	}
}

void Ambience::set_fog_color(Math::Color3 color) {
	m_fogColor = std::move(color);
}

void Ambience::set_fog_start(float fogStart) {
	m_fogStart = fogStart;
}

void Ambience::set_fog_end(float fogEnd) {
	m_fogEnd = fogEnd;
}

std::string Ambience::get_time_of_day() const {
	unsigned uMins = static_cast<unsigned>(m_minutesAfterMidnight);
	unsigned hours = uMins / 60;
	unsigned minutes = uMins - (hours * 60);
	unsigned seconds = uMins - (hours * 60) - minutes;

	char buffer[9] = {};
	sprintf(buffer, "%02u:%02u:%02u", hours, minutes, seconds);

	return std::string(buffer);
}

Math::Vector3 Ambience::get_sun_direction() const {
	float dayAngle = static_cast<float>((m_minutesAfterMidnight) * 2.0 * M_PI / (24.0 * 60.0));
	float latAngle = Math::radians(-90.f - (m_geographicLatitude - 23.5f));

	float cosD = Math::cos(dayAngle);
	float sinD = Math::sin(dayAngle);

	float cosL = Math::cos(latAngle);
	float sinL = Math::sin(latAngle);

	return -Math::Vector3(sinD * sinL, -sinL * cosD, cosL);
}

ECS::Entity Ambience::get_active_skybox() const {
	return m_activeSkybox;
}


