#pragma once
typedef struct
{
	i32 cursor_x;
	i32 cursor_y;
} Board;
extern Board board;

void board_init();
void board_render();

void cursor_move(i32 dx, i32 dy);
