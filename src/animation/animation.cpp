#include "animation.hpp"

#include <algorithm>

using namespace Game;

Animation::Animation(std::string_view name)
		: m_name(std::move(name))
		, m_numFrames(0)
		, m_duration(0.f) {}

void Animation::add_frame(const std::string& name, float time, Math::Vector3 position,
		Math::Quaternion rotation, Math::Vector3 scale) {
	auto& channel = m_channels[name];

	channel.push_back({time, Math::BoneTransform{std::move(position), std::move(rotation),
			std::move(scale)}});

	std::sort(channel.begin(), channel.end(), [](auto& a, auto& b) {
		return a.time < b.time;
	});

	if (m_numFrames < channel.size()) {
		m_numFrames = channel.size();
	}

	if (m_duration < time) {
		m_duration = time;
	}
}

void Animation::get_transform(const std::string& name, float time, Math::BoneTransform& result) const {
	auto& channel = m_channels.find(name)->second;

	if (channel.size() == 1) {
		result = channel[0].transform;
	}
	else {
		auto begin = channel.begin();
		auto end = channel.end();

		BoneKeyframe target;
		target.time = time;

		auto it = std::lower_bound(begin, end, target, [](auto& a, auto& b) {
			return a.time < b.time;
		});

		if (it == begin) {
			auto& frame0 = channel[0];
			auto& frame1 = channel[1];
			auto lerpAmt = (time - frame0.time) / (frame1.time - frame0.time);
			frame0.transform.mix(frame1.transform, lerpAmt, result);
		}
		else if (it == end) {
			result = channel.back().transform;
		}
		else {
			auto& frame0 = *(it - 1);
			auto& frame1 = *it;
			auto lerpAmt = (time - frame0.time) / (frame1.time - frame0.time);
			frame0.transform.mix(frame1.transform, lerpAmt, result);
		}
	}
}

size_t Animation::get_num_frames() const {
	return m_numFrames;
}

float Animation::get_duration() const {
	return m_duration;
}

/*void Animation::add_keyframe(KeyFrame&& keyframe) {
	m_keyframes.push_back(std::move(keyframe));

	std::sort(m_keyframes.begin(), m_keyframes.end(), [](auto& a, auto& b) {
		return a.get_time() < b.get_time();
	});
}

const KeyFrame& Animation::get_keyframe(size_t index) const {
	return m_keyframes[index];
}

size_t Animation::get_num_frames() const {
	return m_keyframes.size();
}*/

const std::string& Animation::get_name() const {
	return m_name;
}

