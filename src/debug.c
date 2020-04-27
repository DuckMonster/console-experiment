#include "debug.h"
#include "winmin.h"
#include <stdlib.h>
#include <stdio.h>

char* parse_vargs(const char* format, va_list list)
{
	int msg_length = vsnprintf(NULL, 0, format, list);

	char* msg_buffer = (char*)malloc(msg_length + 1);
	vsprintf(msg_buffer, format, list);

	return msg_buffer;
}

void _debug_log(const char* format, ...)
{
	va_list vl;
	va_start(vl, format);
	char* msg = parse_vargs(format, vl);
	va_end(vl);

	printf("%s\n", msg);

	free(msg);
}

void _msg_box(const char* title, const char* format, ...)
{
	va_list vl;
	va_start(vl, format);
	char* msg = parse_vargs(format, vl);
	va_end(vl);

	MessageBox(NULL, msg, title, MB_OK);

	free(msg);
}

bool _can_debug_break()
{
	return IsDebuggerPresent();
}

void _debug_exit(i32 exit_code)
{
	exit(exit_code);
}