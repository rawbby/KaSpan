#pragma once

#include <cstdint>

#define LOCK(vert, lock) while (!__sync_bool_compare_and_swap(lock + vert, 0, -1))
#define UNLOCK(vert, lock) lock[vert] = 0

typedef std::int64_t vertex_t;
typedef std::int64_t index_t;
typedef double path_t;
typedef std::int64_t depth_t;
typedef std::int64_t color_t;
typedef std::int64_t long_t;

#define NEGATIVE (int)-1
#define ORPHAN (unsigned char)254
#define UNVIS (long)-1

#define TRIM_TIMES 3
