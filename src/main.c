#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include "context.h"
#include "cells.h"
#include "board.h"
#include "gl_bind.h"

int main()
{
	_chdir("..\\..");

	context_open("Console Game", 100, 100, CELL_WIDTH * CELL_COLS * 3, CELL_HEIGHT * CELL_ROWS * 3);
	cells_init();

	while(context_is_open())
	{
		context_begin_frame();

		glClearColor(0.1f, 0.1f, 0.1f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		board_render();
		cells_render();

		context_end_frame();
	}
	return 0;
}