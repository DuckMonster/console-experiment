#include "types.h"

bool point_in_rect(Point point, Rect rect)
{
	return (point.x >= rect.min.x && point.x <= rect.max.x &&
			point.y >= rect.min.y && point.y <= rect.max.y);
}