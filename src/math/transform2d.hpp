#pragma once

#include <cassert>

#include <math/trigonometric.hpp>
#include <math/geometric.hpp>

#include <math/vector2.hpp>
#include <math/vector3.hpp>
#include <math/matrix2x2.hpp>

namespace Math {

class Transform2D {
	public:
		Transform2D() = default;
		Transform2D(float diagonal)
				: m_columns{{diagonal, 0.f}, {0.f, diagonal}, {}} {}

		 Transform2D& set_translation(const Vector2& translation) {
			m_columns[2] = translation;

			return *this;
		}

		 Transform2D& set_translation(float x, float y) {
			m_columns[2][0] = x;
			m_columns[2][1] = y;

			return *this;
		}

		 Transform2D& set_rotation(float rotation) {
			float s = Math::sin(rotation);
			float c = Math::cos(rotation);

			m_columns[0][0] = c;
			m_columns[0][1] = s;
			m_columns[1][0] = -s;
			m_columns[1][1] = c;

			return *this;
		}

		 Transform2D& apply_scale(float scale) {
			m_columns[0] *= scale;
			m_columns[1] *= scale;

			return *this;
		}

		 Transform2D& apply_scale_local(const Vector2& scale) {
			m_columns[0] *= scale.x;
			m_columns[1] *= scale.y;

			return *this;
		}

		 Transform2D& apply_scale_global(const Vector2& scale) {
			m_columns[0] *= scale;
			m_columns[1] *= scale;

			return *this;
		}

		Transform2D& fast_inverse_self() {
			get_rotation_matrix() = Math::transpose(get_rotation_matrix());
			m_columns[2] *= -1.f;

			return *this;
		}

		Transform2D fast_inverse() const {
			return Transform2D(*this).fast_inverse_self();
		}

		Vector2 point_to_local_space(const Vector2& point) const {
			return Math::transpose(get_rotation_matrix()) * (point - m_columns[2]);
		}

		Vector2& operator[](size_t index) {
			return m_columns[index];
		}

		const Vector2& operator[](size_t index) const {
			return m_columns[index];
		}

		Transform2D& operator+=(const Vector2& c) {
			m_columns[2] += c;

			return *this;
		}

		Transform2D& operator-=(const Vector2& c) {
			m_columns[2] -= c;

			return *this;
		}

		Transform2D& operator*=(const Transform2D& c) {
			m_columns[2] += get_rotation_matrix() * c.m_columns[2];
			get_rotation_matrix() *= c.get_rotation_matrix();

			return *this;
		}

		Vector3 row(size_t index) const {
			switch (index) {
				case 0:
					return Vector3(m_columns[0][0], m_columns[1][0], m_columns[2][0]);
				case 1:
					return Vector3(m_columns[0][1], m_columns[1][1], m_columns[2][1]);
				default:
					return Vector3();
			}
		}

		Matrix2x2& get_rotation_matrix() {
			return *reinterpret_cast<Matrix2x2*>(this);
		}

		const Matrix2x2& get_rotation_matrix() const {
			return *reinterpret_cast<const Matrix2x2*>(this);
		}
	private:
		Vector2 m_columns[3];
};

}

inline Math::Transform2D operator*(const Math::Transform2D& a,
		const Math::Transform2D& b) {
	auto result = a;
	result *= b;

	return result;
}

inline Math::Vector2 operator*(const Math::Transform2D& c, const Math::Vector3& v) {
	return Math::Vector2(Math::dot(c.row(0), v), Math::dot(c.row(1), v));
}

inline Math::Transform2D operator+(const Math::Transform2D& c, const Math::Vector2& v) {
	auto result = c;
	result += v;

	return result;
}

inline Math::Transform2D operator-(const Math::Transform2D& c, const Math::Vector2& v) {
	auto result = c;
	result -= v;

	return result;
}

