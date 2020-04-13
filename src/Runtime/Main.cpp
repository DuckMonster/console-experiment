#include <stdio.h>
#include <stdlib.h>
#include "Core/Context/Context.h"
#include "Font/Font.h"

int main(int arg_num, const char** args)
{
	context_open("Console Game", 100, 100, 400, 300);

	Font font;
	font_load(&font, "res/consola.ttf");

	while(context.is_open)
	{
		context_begin_frame();
		context_end_frame();
	}

	return 0;
}