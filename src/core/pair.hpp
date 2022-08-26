#pragma once

template <typename K, typename V>
struct Pair {
	K first;
	V second;
};

template <typename K, typename V>
Pair(K, V) -> Pair<K, V>;

