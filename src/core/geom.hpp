#pragma once

#include <ecs/ecs_fwd.hpp>

#include <core/normal_id.hpp>
#include <core/surface_type.hpp>
#include <core/geom_type.hpp>

#include <math/vector3.hpp>
#include <math/color3uint8.hpp>
#include <math/transform.hpp>

namespace Game {

struct Instance;

class Geometry {
	public:
		static void create(ECS::Manager&, ECS::Entity);
		static void on_destroyed(ECS::Manager&, Instance&, ECS::Entity);
		static void on_ancestry_changed(ECS::Manager&, Instance&, ECS::Entity, Instance*,
				ECS::Entity);

		void set_transform(ECS::Entity selfEntity, Math::Transform);
		void set_size(ECS::Entity selfEntity, Math::Vector3);
		void set_color(ECS::Entity selfEntity, Math::Color3uint8);
		void set_transparency(ECS::Entity selfEntity, float);
		void set_reflectance(ECS::Entity selfEntity, float);

		void set_surface_type(ECS::Entity, NormalId, SurfaceType);
		void set_shape(ECS::Entity selfEntity, GeomType);

		const Math::Transform& get_transform() const;
		const Math::Vector3& get_size() const;
		Math::Color3uint8 get_color() const;
		
		float get_transparency() const;
		float get_reflectance() const;

		SurfaceType get_surface_type(NormalId face) const;
		GeomType get_shape() const;
	private:
		Math::Transform m_transform;
		Math::Vector3 m_size;
		Math::Color3uint8 m_color;
		float m_transparency;
		float m_reflectance;
		uint32_t m_surfaceTypes;
		GeomType m_shape;
};

}

