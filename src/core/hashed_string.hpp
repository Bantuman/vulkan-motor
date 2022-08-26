#pragma once

// FNV-11 hash or something, TODO: look this up
constexpr uint64_t fast_string_hash(const char* str) {
	uint64_t hashVal = 0xcbf29ce484222325ULL;

	while (*str) {
		hashVal *= 0x100000001b3ULL;
		hashVal ^= *str++;
	}

	return hashVal;
}

constexpr uint64_t operator "" _hs(const char* str, size_t) {
	return fast_string_hash(str);
}

