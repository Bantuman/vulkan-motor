#include "range_builder.hpp"

#include <algorithm>

static constexpr bool overlaps(const RangeBuilder::Range& a, const RangeBuilder::Range& b) {
	return a.max < b.min;
}

void RangeBuilder::add(uint32_t index) {
	add({index, index + 1});
}

void RangeBuilder::add(RangeBuilder::Range range) {
	auto begin = m_ranges.begin();
	auto end = m_ranges.end();
	auto maxBound = std::lower_bound(begin, end, range);

	if (maxBound != end) {
		if (range.min >= maxBound->min) {
			return;
		}
		else if (range.max >= maxBound->min && range.min < maxBound->min) {
			maxBound->min = range.min;
		}
	}
	else {
		m_ranges.emplace_back(std::move(range));
		begin = m_ranges.begin();
		end = m_ranges.end();
		maxBound = end - 1;
	}

	auto minBound = std::lower_bound(begin, end, range, overlaps);

	if (minBound != end && range.max >= minBound->min) {
		if (range.min < minBound->min) {
			minBound->min = range.min;
		}

		minBound->max = maxBound->max;
		++minBound;
		++maxBound;
		m_ranges.erase(minBound, maxBound);
	}
	else {
		m_ranges.emplace(maxBound, std::move(range));
	}
}

void RangeBuilder::clear() {
	m_ranges.clear();
}

bool RangeBuilder::empty() const {
	return m_ranges.empty();
}

RangeBuilder::Container::const_iterator RangeBuilder::begin() const {
	return m_ranges.cbegin();
}

RangeBuilder::Container::const_iterator RangeBuilder::end() const {
	return m_ranges.cend();
}

