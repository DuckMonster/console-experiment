#pragma once

typedef struct
{
	i32 x;
	i32 y;
	u32 width;
	u32 height;
} Context;

void context_open(const char* title, i32 x, i32 y, u32 width, u32 height);
bool context_is_open();
void context_close();

void context_begin_frame();
void context_end_frame();

// Current times since init in milliseconds
float time_now();