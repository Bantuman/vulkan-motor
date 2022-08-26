#pragma once

#include <cstdint>

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

#include <math/bone_transform.hpp>

namespace Game {

class Animation {
	public:
		explicit Animation(std::string_view name);

		void add_frame(const std::string& name, float time, Math::Vector3 position,
				Math::Quaternion rotation, Math::Vector3 scale);
		
		void get_transform(const std::string& name, float time, Math::BoneTransform& result) const;

		size_t get_num_frames() const;
		float get_duration() const;

		const std::string& get_name() const;
	private:
		struct BoneKeyframe {
			float time;
			Math::BoneTransform transform;
		};
		
		std::string m_name;
		std::unordered_map<std::string, std::vector<BoneKeyframe>> m_channels;
		size_t m_numFrames;
		float m_duration;
};

}

