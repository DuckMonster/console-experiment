#pragma once

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

typedef struct
{
	Point a;
	Point b;
} Rect;
inline Rect rect(Point a, Point b)
{
	Rect rect;
	rect.a.x = min(a.x, b.x);
	rect.b.x = max(a.x, b.x);
	rect.a.y = min(a.y, b.y);
	rect.b.y = max(a.y, b.y);
	return rect;
}

bool point_in_rect(Point point, Rect rect);