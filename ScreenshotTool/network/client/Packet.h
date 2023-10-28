#pragma once

#include <stdint.h>
#include <vector>

struct Packet {
	using size_t = uint32_t;

	size_t size;
	const char* payload;
};