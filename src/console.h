#pragma once

struct Console
{
	u32 x;
	u32 y;
	u32 cols;
	u32 rows;
};

void console_open(const char* title, u32 x, u32 y, u32 cols, u32 rows);