#pragma once

#include <Windows.h>

#include <vector>

struct Packet {
	using size_t = ULONG;

	size_t biWidth;
	size_t biHeight;
	size_t biBitCount;
	size_t payloadSize;
	BYTE* payload;
};