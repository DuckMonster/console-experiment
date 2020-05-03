#pragma once
#include <string.h>

#define mem_zero(ptr, size) (memset(ptr, 0, size))
#define zero_t(expr) (mem_zero(&expr, sizeof(expr)))