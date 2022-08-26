#pragma once

#include <string>

#include <ecs/ecs_fwd.hpp>

#include <core/geom_type.hpp>
#include <core/normal_id.hpp>

#include <math/color3.hpp>

namespace Game {

struct Instance;
class Geometry;

class ImagePlane {
	public:
		static void create(ECS::Manager&, ECS::Entity);
		static void on_destroyed(ECS::Manager&, Instance&, ECS::Entity);
		static void on_ancestry_changed(ECS::Manager&, Instance&, ECS::Entity, Instance*,
				ECS::Entity);

		void set_texture(ECS::Manager&, Instance& selfInstance, ECS::Entity selfEntity,
				std::string);
		void set_color(ECS::Entity selfEntity, Math::Color3);
		void set_transparency(ECS::Manager&, Instance& selfInstance, ECS::Entity selfEntity,
				float);
		void set_z_index(ECS::Entity selfEntity, int32_t);
		void set_face(ECS::Manager&, Instance& selfInstance, ECS::Entity selfEntity, NormalId);

		bool is_visible() const;
	private:
		std::string m_texture;
		Math::Color3 m_color;
		float m_transparency;
		uint32_t m_zIndex;
		NormalId m_face;
		GeomType m_shape;
		bool m_validTexture;
		bool m_renderable;

		void full_redraw(const Geometry&, ECS::Entity);
};

}

