#pragma once

#include <cstdint>

#include <vector>

class RangeBuilder {
	public:
		struct Range {
			uint32_t min;
			uint32_t max;

			constexpr bool operator<(const Range& other) const {
				return max < other.max;
			}
		};

		using Container = std::vector<Range>;

		void add(uint32_t index);
		void add(Range);
		void clear();

		bool empty() const;

		Container::const_iterator begin() const;
		Container::const_iterator end() const;
	private:
		Container m_ranges;
};

