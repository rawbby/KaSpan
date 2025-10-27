#pragma once

#if defined(KASPAN_DEBUG) || defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
#define KASPAN_DEBUG 1
#else
#define KASPAN_DEBUG 0
#endif
