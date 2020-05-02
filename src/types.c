#include "types.h"

bool point_in_rect(Point point, Rect rect)
{
	return (point.x >= rect.a.x && point.x <= rect.b.x &&
			point.y >= rect.a.y && point.y <= rect.b.y);
}