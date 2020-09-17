#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <cstdint>
#include <random>

#if INTPTR_MAX == INT64_MAX
#define VEXCORE_x64
#elif INTPTR_MAX == INT32_MAX
#define ECSCORE_x32
#else
#error Unknown ptr size, abort
#endif

#pragma warning(push)
#pragma warning(disable : 26495)
#pragma warning(disable : 26451)
#pragma warning(disable : 28812)
#include <VCore/Dependencies/MurmurHash3.h>
#pragma warning(pop)

namespace vex
{
	namespace util
	{
		static constexpr int gPrimeNumbers[] = {3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293,
			353, 431, 521, 631, 761, 919, 1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103,
			12143, 14591, 17321, 21269, 25253, 31393, 39769, 49157, 62851, 90523, 108631, 130363, 156437, 187751,
			225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 946037, 1395263, 1572869, 2009191, 2411033,
			2893249, 3471899, 4166287, 4999559, 5999471, 7199369};

		static constexpr int gPrimeSize = sizeof(gPrimeNumbers) / sizeof(int);

		inline constexpr int FindUpperBound(const int* a, int n, int x)
		{
			if (x > a[n - 1])
				return a[n - 1];

			int iter = 0;
			int count = n;
			while (iter < count)
			{
				int mid = (iter + count) / 2;
				if (x > a[mid])
				{
					iter = mid + 1;
				}
				else
				{
					count = mid;
				}
			}

			return a[iter];
		}

		inline constexpr int ClosestPrimeSearch(int value) { return FindUpperBound(gPrimeNumbers, gPrimeSize, value); }

		inline int RandomRange(int fromInc, int toExc)
		{
			static std::random_device rd;
			static std::mt19937 mt(rd());
			std::uniform_int_distribution<int> dist(fromInc,
				toExc - 1); // exclusive, as it is more common use case
			return dist(mt);
		}

		static const std::size_t fnv_prime = 16777619u;
		static const std::size_t fnv_offset_basis = 2166136261u;

		static inline int fnv1a(std::string const& text)
		{
			std::size_t hash = fnv_offset_basis;
			for (std::string::const_iterator it = text.begin(), end = text.end(); it != end; ++it)
			{
				hash ^= *it;
				hash *= fnv_prime;
			}

			return (int)hash;
		}

		static inline int Hash(char* c, int sz) { return (int)murmur::MurmurHash3_x86_32(c, sz); }

		struct SHash
		{
			static inline int Hash(const std::string& str)
			{
#ifdef ECSCORE_x64
				return (int)murmur::MurmurHash3_x86_32(str.data(), (int)str.size());
#else
				return fnv1a(str);
#endif
			}
		};
		struct SHash_STD
		{
			static inline int Hash(const std::string& str) { return (int)std::hash<std::string>{}(str); }
		};
		struct SHash_FNV1a
		{
			static inline int Hash(const std::string& str) { return fnv1a(str); }
		};
		struct SHash_MURMUR
		{
			static inline int Hash(const std::string& str)
			{
				return (int)murmur::MurmurHash3_x86_32(str.data(), (int)str.size());
			}
		};
	} // namespace util
} // namespace vex
