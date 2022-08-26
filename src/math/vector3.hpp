#pragma once

#include <glm/vec3.hpp>

namespace Math {

using Vector3 = glm::vec3;
static inline Vector3 Vector3Lerp(const Vector3& vec0, const Vector3& vec1, float delta){
	return {vec0.x + (vec1.x - vec0.x) * delta, vec0.y + (vec1.y - vec0.y) * delta, vec0.z + (vec1.z - vec0.z) * delta};
}
}

