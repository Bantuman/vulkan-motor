#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <math/transform.hpp>

namespace Game {

class KeyFrame {
	public:
		explicit KeyFrame(float time)
				: m_time(time) {}

		float get_time() const {
			return m_time;
		}

		void add_bone_transform(std::string_view name, Math::Vector3 position,
				Math::Quaternion rotation, Math::Vector3 scale) {
			m_boneTransforms.emplace(std::make_pair(std::string(std::move(name)),
					Math::Transform{std::move(position), std::move(rotation), std::move(scale)}));
		}

		const Math::Transform& get_bone_transform(const std::string& name) const {
			return m_boneTransforms.find(name)->second;
		}
	private:
		float m_time;
		std::unordered_map<std::string, Math::Transform> m_boneTransforms;
};

}

