#include "context.h"
#include "cells.h"

void prompt_msg(const char* title, const char* msg)
{
	while(context_is_open())
	{
		context_begin_frame();

		for(i32 y=4; y<10; ++y)
		{
			for(i32 x=4; x<15; ++x)
			{
				cell_set(point(x, y), '#', CLR_WHITE, CLR_BLACK);
			}
		}

		cell_write_str(point(5, 5), msg, -1, -1);
		cells_render();

		context_end_frame();
	}
}