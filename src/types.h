#pragma once
#include <stdlib.h>

typedef char bool;
enum { false, true };

typedef signed char i8;
typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef signed int i32;
typedef unsigned int u32;

typedef float f32;
typedef double d32;

typedef struct
{
	i32 x;
	i32 y;
} Point;
inline Point point(i32 x, i32 y)
{
	Point point;
	point.x = x;
	point.y = y;
	return point;
}
inline bool point_eq(Point a, Point b) { return a.x == b.x && a.y == b.y; }
inline Point point_add(const Point a, const Point b) { return point(a.x + b.x, a.y + b.y); }
inline Point point_sub(const Point a, const Point b) { return point(a.x - b.x, a.y - b.y); }
inline Point point_inv(const Point pt) { return point(-pt.x, -pt.y); }

typedef struct
{
	Point min;
	Point max;
} Rect;
inline Rect rect(Point a, Point b)
{
	Rect rect;
	rect.min.x = min(a.x, b.x);
	rect.max.x = max(a.x, b.x);
	rect.min.y = min(a.y, b.y);
	rect.max.y = max(a.y, b.y);
	return rect;
}

bool point_in_rect(Point point, Rect rect);