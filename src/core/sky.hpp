#pragma once

#include <string>

#include <ecs/ecs_fwd.hpp>

namespace Game {

struct Instance;

class Sky {
	public:
		static void create(ECS::Manager&, ECS::Entity);
		static void on_destroyed(ECS::Manager&, Instance&, ECS::Entity);
		static void on_ancestry_changed(ECS::Manager&, Instance&, ECS::Entity, Instance*,
				ECS::Entity);

		void set_back_id(std::string);
		void set_front_id(std::string);
		void set_left_id(std::string);
		void set_right_id(std::string);
		void set_up_id(std::string);
		void set_down_id(std::string);

		const std::string& get_back_id() const;
		const std::string& get_front_id() const;
		const std::string& get_left_id() const;
		const std::string& get_right_id() const;
		const std::string& get_up_id() const;
		const std::string& get_down_id() const;
	private:
		std::string m_skyboxBackID;
		std::string m_skyboxFrontID;
		std::string m_skyboxLeftID;
		std::string m_skyboxRightID;
		std::string m_skyboxUpID;
		std::string m_skyboxDownID;
};

}

