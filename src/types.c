#include "types.h"

bool point_in_rect(Point point, Rect rect)
{
	return (point.x >= rect.min.x && point.x <= rect.max.x &&
			point.y >= rect.min.y && point.y <= rect.max.y);
}

bool rect_rect_intersect(Rect a, Rect b)
{
	return (a.max.x >= b.min.x &&
		b.max.x >= a.min.x && 
		a.max.y >= b.min.y &&
		b.max.y >= a.min.y);
}