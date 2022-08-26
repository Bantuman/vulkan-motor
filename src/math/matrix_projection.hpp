#pragma once

#include <math/vector4.hpp>
#include <math/matrix4x4.hpp>

#include <math/trigonometric.hpp>

namespace Math {

// for Vulkan reverse-Z
inline Math::Matrix4x4 infinite_perspective(float fov, float aspectRatio, float zNear) {
	float tanHalfFOV = Math::tan(fov * 0.5f);

	return Math::Matrix4x4(
		1.f / (tanHalfFOV * aspectRatio), 0.f, 0.f, 0.f,
		0.f, -1.f / tanHalfFOV, 0.f, 0.f,
		0.f, 0.f, 0.f, -1.f,
		0.f, 0.f, zNear, 0.f
	);
}

inline Math::Matrix4x4 inverse_infinite_perspective(float fov, float aspectRatio, float zNear) {
	float tanHalfFOV = Math::tan(fov * 0.5f);

	return Math::Matrix4x4(
		tanHalfFOV * aspectRatio, 0.f, 0.f, 0.f,
		0.f, -tanHalfFOV, 0.f, 0.f,
		0.f, 0.f, 0.f, 1.f / zNear,
		0.f, 0.f, -1.f, 0.f
	);
}

// values (x, y, z, w) such that:
// vec3(fma(gl_FragCoord.xy * vec2(1.0, -1.0), values.xx, values.yz), values.w) / depth
// will be equivalent to:
// vec4 h = (inverse(perspective) * vec4(clipSpaceFragCoord, depth, 1.0));
// h / h.w;
inline Math::Vector4 infinite_perspective_clip_to_view_space_deprojection(float fov,
		float width, float height, float zNear) {
	float tanHalfFOV = Math::tan(fov * 0.5f);

	return Math::Vector4(2.f * zNear * tanHalfFOV / height, -zNear * tanHalfFOV * width / height,
			zNear * tanHalfFOV, -zNear);
}

// The size in pixels of a 1 unit object if viewed from 1 unit away
float image_plane_pixels_per_unit(float fov, float width) {
	float scale = -2.f * Math::tan(fov * 0.5f);

	return width / scale;
}

}

