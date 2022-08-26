#include "bone_transform.hpp"

#include <glm/gtx/transform.hpp>

#include <math/common.hpp>

using namespace Math;

static Math::Quaternion my_nlerp(const Math::Quaternion& from, const Math::Quaternion& to,
		float inc, bool shortest) {
	if (shortest && glm::dot(from, to) < 0.f) {
		return glm::normalize(from + (-to - from) * inc);
	}
	else {
		return glm::normalize(from + (to - from) * inc);
	}
}

static Math::Quaternion my_slerp(const Math::Quaternion& from, const Math::Quaternion& to,
		float inc, bool shortest) {
	float cs = glm::dot(from, to);
	Math::Quaternion correctedTo = to;

	if (shortest && cs < 0.f) {
		cs = -cs;
		correctedTo = -to;
	}

	if (cs >= 0.9999f || cs <= -0.9999f) {
		return my_nlerp(from, correctedTo, inc, false);
	}

	float sn = sqrt(1.f - cs * cs);
	float angle = atan2(sn, cs);
	float invSin = 1.f / sn;

	float srcFactor = sin((1.f - inc) * angle) * invSin;
	float dstFactor = sin(inc * angle) * invSin;

	return from * srcFactor + correctedTo * dstFactor;
}

void BoneTransform::mix(const BoneTransform& other, float factor, BoneTransform& dest) const {
	dest.position = glm::mix(position, other.position, factor);
	//dest.rotation = glm::mix(rotation, other.rotation, factor);
	dest.rotation = my_slerp(rotation, other.rotation, factor, true);
	dest.scale = glm::mix(scale, other.scale, factor);
}

Math::Transform BoneTransform::to_transform() const {
	return Transform(position, rotation).scale_self_by(scale);
}

Math::Matrix4x4 BoneTransform::to_matrix() const {
	return glm::scale(glm::translate(Matrix4x4(1.f), position) * glm::mat4_cast(rotation), scale);
}

