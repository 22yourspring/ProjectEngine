#pragma once

#include <cstring>
#include <cstddef>

struct FMemory
{
	static void* Memcpy(void* Destination, const void* Source, std::size_t Count)
	{
		return std::memcpy(Destination, Source, Count);
	}

	static void* Memzero(void* Destination, std::size_t Count)
	{
		return std::memset(Destination, 0, Count);
	}

	static void* Memset(void* Destination, int Value, std::size_t Count)
	{
		return std::memset(Destination, Value, Count);
	}
};
