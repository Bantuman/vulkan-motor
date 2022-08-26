#pragma once

#include <math/common.hpp>
#include <math/geometric.hpp>
#include <math/trigonometric.hpp>
#include <math/vector3.hpp>
#include <math/vector4.hpp>
#include <math/quaternion.hpp>
#include <math/matrix3x3.hpp>
#include <math/matrix4x4.hpp>

namespace Math {

class Transform {
	public:
		static Transform from_axis_angle(const Vector3& axis, float angle);
		static Transform from_euler_angles_xyz(float rx, float ry, float rz);
		static Transform look_at(const Vector3& eye, const Vector3& center,
				const Vector3& up = Vector3(0, 1, 0));

		// FIXME: This needs to be  but GLM doesn't have  enabled for some reason
		Transform() = default;

		explicit Transform(float diagonal)
				: m_columns{{diagonal, 0.f, 0.f}, {0.f, diagonal, 0.f}, {0.f, 0.f, diagonal}, {}}
			{}

		Transform(float x, float y, float z)
				: m_columns{{1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {x, y, z}} {}

		explicit Transform(Vector3 pos)
				: m_columns{{1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, std::move(pos)} {}

		Transform(Vector3 pos, const Quaternion& rot)
				: m_columns{{}, {}, {}, std::move(pos)} {
			get_rotation_matrix() = glm::mat3_cast(rot);
		}

		Transform(Vector3 pos, Vector3 vX, Vector3 vY, Vector3 vZ)
				: m_columns{std::move(vX), std::move(vY), std::move(vZ), std::move(pos)} {}

		Transform(float x, float y, float z, float r00, float r01, float r02, float r10, float r11,
					float r12, float r20, float r21, float r22)
				: m_columns{{r00, r01, r02}, {r10, r11, r12}, {r20, r21, r22}, {x, y, z}} {}

		explicit Transform(const Matrix4x4& m)
				: m_columns{Vector3(m[0]), Vector3(m[1]), Vector3(m[2]), Vector3(m[3])} {}

		Transform& operator+=(const Vector3& v) {
			get_position() += v;
			return *this;
		}

		Transform& operator-=(const Vector3& v) {
			get_position() -= v;
			return *this;
		}

		Transform& operator*=(const Transform& other) {
			get_position() += get_rotation_matrix() * other.get_position();
			get_rotation_matrix() *= other.get_rotation_matrix();

			return *this;
		}

		Vector3 operator*(const Vector3& v) const {
			return get_rotation_matrix() * v + get_position();
		}

		Transform& scale_self_by(const Vector3& v) {
			get_rotation_matrix()[0] *= v.x;
			get_rotation_matrix()[1] *= v.y;
			get_rotation_matrix()[2] *= v.z;

			return *this;
		}

		Transform scale_by(const Vector3& v) const {
			return Transform(*this).scale_self_by(v);
		}

		Transform& fast_inverse_self() {
			get_rotation_matrix() = glm::transpose(get_rotation_matrix());
			m_columns[3] = get_rotation_matrix() * -m_columns[3];

			return *this;
		}

		Transform& inverse_self() {
			get_rotation_matrix() = glm::inverse(get_rotation_matrix());
			m_columns[3] = get_rotation_matrix() * -m_columns[3];

			return *this;
		}

		Transform fast_inverse() const {
			return Transform(*this).fast_inverse_self();
		}

		Transform inverse() const {
			return Transform(*this).inverse_self();
		}

		 Vector3& operator[](size_t index) {
			return m_columns[index];
		}

		 const Vector3& operator[](size_t index) const {
			return m_columns[index];
		}

		Matrix3x3& get_rotation_matrix() {
			return *reinterpret_cast<Matrix3x3*>(this);
		}

		const Matrix3x3& get_rotation_matrix() const {
			return *reinterpret_cast<const Matrix3x3*>(this);
		}

		 Vector3& get_position() {
			return m_columns[3];
		}

		const Vector3& get_position() const {
			return m_columns[3];
		}

		const Vector3& right_vector() const {
			return m_columns[0];
		}

		const Vector3& up_vector() const {
			return m_columns[1];
		}

		 Vector3 look_vector() const {
			return -m_columns[2];
		}

		 Matrix4x4 to_matrix4x4() const {
			return Matrix4x4(Vector4(m_columns[0], 0.f), Vector4(m_columns[1], 0.f),
					Vector4(m_columns[2], 0.f), Vector4(m_columns[3], 1.f));
		}

		 void to_euler_angles_xyz(float& x, float& y, float& z) const {
			float t1 = Math::atan2(m_columns[2][1], m_columns[2][2]);
			float c2 = Math::sqrt(m_columns[0][0] * m_columns[0][0]
					+ m_columns[1][0] * m_columns[1][0]);
			float t2 = Math::atan2(-m_columns[2][0], c2);
			float s1 = Math::sin(t1);
			float c1 = Math::cos(t1);
			float t3 = Math::atan2(s1 * m_columns[0][2] - c1 * m_columns[0][1],
					c1 * m_columns[1][1] - s1 * m_columns[1][2]);

			x = -t1;
			y = -t2;
			z = -t3;
		}
	private:
		Vector3 m_columns[4];
};

}

inline Math::Transform operator*(const Math::Transform& a, const Math::Transform& b) {
	auto result = a;
	result *= b;

	return result;
}

inline Math::Transform Math::Transform::from_axis_angle(const Math::Vector3& axis, float angle) {
	float c = Math::cos(angle);
	float s = Math::sin(angle);
	float t = 1.f - c;

	float r00 = c + axis.x * axis.x * t;
	float r11 = c + axis.y * axis.y * t;
	float r22 = c + axis.z * axis.z * t;

	float tmp1 = axis.x * axis.y * t;
	float tmp2 = axis.z * s;

	float r01 = tmp1 + tmp2;
	float r10 = tmp1 - tmp2;

	float tmp3 = axis.x * axis.z * t;
	float tmp4 = axis.y * s;

	float r02 = tmp3 - tmp4;
	float r20 = tmp3 + tmp4;

	float tmp5 = axis.y * axis.z * t;
	float tmp6 = axis.x * s;
	
	float r12 = tmp5 + tmp6;
	float r21 = tmp5 - tmp6;

	return Transform(0.f, 0.f, 0.f, r00, r01, r02, r10, r11, r12, r20, r21, r22);
}

inline Math::Transform Math::Transform::from_euler_angles_xyz(float rX, float rY, float rZ) {
	return from_axis_angle(Vector3(0, 0, 1), rZ)
			* from_axis_angle(Vector3(0, 1, 0), rY)
			* from_axis_angle(Vector3(1, 0, 0), rX);
}

inline Math::Transform Math::Transform::look_at(const Vector3& eye, const Vector3& center,
		const Vector3& up) {
	auto fwd = Math::normalize(eye - center);
	auto right = Math::normalize(Math::cross(up, fwd));
	auto actualUp = Math::cross(fwd, right);

	return Math::Transform(eye, right, actualUp, fwd);
}
