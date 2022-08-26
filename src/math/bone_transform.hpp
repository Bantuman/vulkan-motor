#pragma once

#include <math/vector3.hpp>
#include <math/quaternion.hpp>
#include <math/matrix4x4.hpp>
#include <math/transform.hpp>

namespace Math {

struct BoneTransform {
	void mix(const BoneTransform& other, float factor, BoneTransform& dest) const;

	Math::Transform to_transform() const;
	Math::Matrix4x4 to_matrix() const;

	Math::Vector3 position;
	Math::Quaternion rotation;
	Math::Vector3 scale;
};

}

