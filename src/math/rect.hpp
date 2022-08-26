#pragma once

#include <math/common.hpp>
#include <math/vector2.hpp>

namespace Math {

struct Rect {
	constexpr Rect()
			: xMin(0.f)
			, yMin(0.f)
			, xMax(0.f)
			, yMax(0.f) {}

	constexpr Rect(float xMinIn, float yMinIn, float xMaxIn, float yMaxIn)
			: xMin(xMinIn)
			, yMin(yMinIn)
			, xMax(xMaxIn)
			, yMax(yMaxIn) {}

	Rect(const Math::Vector2& minIn, const Math::Vector2& maxIn)
			: xMin(minIn.x)
			, yMin(minIn.y)
			, xMax(maxIn.x)
			, yMax(maxIn.y) {}

	Rect intersection(const Rect& other) const;
	Vector2 centroid() const;
	Vector2 size() const;

	bool valid() const;
	bool contains(const Math::Vector2& point) const;

	float xMin;
	float yMin;
	float xMax;
	float yMax;
};

}

inline Math::Rect Math::Rect::intersection(const Math::Rect& other) const {
	return Math::Rect(Math::max(xMin, other.xMin), Math::max(yMin, other.yMin),
			Math::min(xMax, other.xMax), Math::min(yMax, other.yMax));
}

inline Math::Vector2 Math::Rect::centroid() const {
	return Math::Vector2((xMin + xMax) * 0.5f, (yMin + yMax) * 0.5f);
}

inline Math::Vector2 Math::Rect::size() const {
	return Math::Vector2(xMax - xMin, yMax - yMin);
}

inline bool Math::Rect::valid() const {
	return xMin < xMax && yMin < yMax;
}

inline bool Math::Rect::contains(const Math::Vector2& point) const {
	return point.x >= xMin && point.x <= xMax && point.y >= yMin && point.y <= yMax;
}

