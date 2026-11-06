#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
using std::min;
using std::max;
inline bool psramFound() { return false; }
inline void *ps_malloc(size_t n) { return malloc(n); }
