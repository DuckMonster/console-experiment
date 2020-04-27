#pragma once

void _debug_log(const char* format, ...);
void _msg_box(const char* title, const char* format, ...);
bool _can_debug_break();
void _debug_exit(i32 exit_code);

#define msg_box(format, ...) (_msg_box("Message", format, __VA_ARGS__))
#define error(format, ...) ((_msg_box("ERROR", format, __VA_ARGS__), 0) || debug_break() || (_debug_exit(1), 0))

#if DEBUG
#define assert(expr) (!!(expr) || (error("Assert failed:\n\n%s(%d)\n%s", __FILE__, __LINE__, #expr), 0))
#define debug_break() (_can_debug_break() && (__debugbreak(), 0))
#define log(format, ...) (_debug_log(format, __VA_ARGS__))
#else
#define assert(expr) expr
#endif