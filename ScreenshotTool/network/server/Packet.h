#pragma once

#include <stdint.h>
#include <vector>

struct Packet {
	using size_t = uint32_t;

	size_t size{};
	std::vector<uint8_t> payload;
};