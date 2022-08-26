#pragma once

#include <string>

#include <core/memory.hpp>

#include <ecs/ecs_fwd.hpp>

#include <math/vector3.hpp>
#include <math/color3uint8.hpp>
#include <math/transform.hpp>

class Mesh;
class RiggedMesh;

namespace Game {

struct Instance;

class MeshGeom {
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

		void set_rigged_mesh(ECS::Manager&, ECS::Entity selfEntity, std::string meshID,
				Memory::SharedPtr<RiggedMesh>, ECS::Entity rig);
		void set_texture(ECS::Entity selfEntity, std::string textureID);

		const Math::Transform& get_transform() const;
		const Math::Vector3& get_size() const;
		Math::Color3uint8 get_color() const;
		
		float get_transparency() const;
		float get_reflectance() const;

		bool is_visible() const;
	private:
		Math::Transform m_transform;
		Math::Vector3 m_size;
		Math::Vector3 m_originalSize;
		Math::Vector3 m_scale;
		Math::Color3uint8 m_color;
		float m_transparency;
		float m_reflectance;

		std::string m_meshID;
		std::string m_textureID;

		Memory::SharedPtr<Mesh> m_mesh;
		ECS::Entity m_rig = ECS::INVALID_ENTITY;
};

}

