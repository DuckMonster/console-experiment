#include "context.h"
#include "gl_bind.h"
#include "board.h"
#include "cells.h"

#define KEY_CTRL 0x1D
#define KEY_SHIFT 0x2A

Context context;

HWND wnd_handle;
HDC wnd_context;
HGLRC gl_context;

bool class_was_registered = false;
const LPCSTR class_name = "WindowClass";
bool is_open = false;

u32 key_mod_flags = 0;
const u32 window_style = (WS_OVERLAPPEDWINDOW | WS_SIZEBOX | WS_VISIBLE);
i32 window_scale = 2;

// Structs for handing events
// WM_KEYDOWN & WM_KEYUP
typedef struct
{
	// Repeat count for this key
	u32 repeat		: 16;
	// Hardware scancode of key
	u32 scancode	: 8;
	// Was extended (CTRL/ALT/etc)
	u32 extended	: 1;
	// Dont touch!
	u32 reserved	: 4;
	// Context code, always 0 for WM_KEYDOWN
	u32 context		: 1;
	// Previous key state (1 if key was down, 0 if it was up)
	u32 previous	: 1;
	// Always 0 
	u32 transition	: 1;
} Win_Key_Params;

// WM_MOUSEMOVE
typedef struct
{
	u16 x;
	u16 y;
} Win_MouseMove_Params;

// WM_SIZE
typedef struct
{
	u16 width;
	u16 height;
} Win_Size_Params;

Point client_to_wnd_delta()
{
	RECT cli_rect;
	cli_rect.left = 0;
	cli_rect.top= 0;
	cli_rect.right = 0;
	cli_rect.bottom = 0;

	AdjustWindowRect(&cli_rect, window_style, false);
	return point(cli_rect.right - cli_rect.left, cli_rect.bottom - cli_rect.top);
}

void client_to_wnd_size(u32* width, u32* height)
{
	Point delta = client_to_wnd_delta();

	*width += delta.x;
	*height += delta.y;
}

void wnd_to_client_size(u32* width, u32* height)
{
	Point delta = client_to_wnd_delta();

	*width -= delta.x;
	*height -= delta.y;
}

LRESULT CALLBACK wnd_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static bool ignore_resize = false;
	switch(msg)
	{
		// Key down
		case WM_KEYDOWN:
		{
			Win_Key_Params* key = (Win_Key_Params*)&lparam;
			log("KEYDOWN: 0x%x", key->scancode);

			if (key->scancode == KEY_CTRL)
				key_mod_flags |= MODK_CTRL;
			if (key->scancode == KEY_SHIFT)
				key_mod_flags |= MODK_SHIFT;

			if (board_key_event(key->scancode, wparam, key_mod_flags))
				break;

			// Quit on escape
			//if (key->scancode == 0x01)
				//SendMessage(wnd, WM_CLOSE, 0, 0);

			break;
		}

		// Key Up
		case WM_KEYUP:
		{
			Win_Key_Params* key = (Win_Key_Params*)&lparam;

			if (key->scancode == KEY_CTRL)
				key_mod_flags &= ~MODK_CTRL;
			if (key->scancode == KEY_SHIFT)
				key_mod_flags &= ~MODK_SHIFT;

			break;
		}

		// Sizing
		case WM_SIZING:
		{
			RECT* size_rect = (RECT*)lparam;

			u32 unit_width = CELL_WIDTH * window_scale;
			u32 unit_height = CELL_HEIGHT * window_scale;

			u32 width = size_rect->right - size_rect->left;
			u32 height = size_rect->bottom - size_rect->top;

			// Convert to client-space
			wnd_to_client_size(&width, &height);

			// Constrain
			width = (u32)((width / unit_width) * unit_width);
			height = (u32)((height / unit_height) * unit_height);

			// Convert back to window-space
			client_to_wnd_size(&width, &height);

			// And adjust the size rect!
			size_rect->right = size_rect->left + width;
			size_rect->bottom = size_rect->top + height;
			break;
		}

		// Size
		case WM_SIZE:
		{
			Win_Size_Params* size = (Win_Size_Params*)&lparam;
			context.width = size->width / (CELL_WIDTH * window_scale);
			context.height = size->height / (CELL_HEIGHT * window_scale);
			glViewport(0, 0, size->width, size->height);

			break;
		}

		// Closing
		case WM_CLOSE:
		{
			is_open = false;
			break;
		}

		// Destroying
		case WM_DESTROY:
		{
			wglDeleteContext(gl_context);
			ReleaseDC(wnd_handle, wnd_context);

			wnd_handle = NULL;
			wnd_context = NULL;
			break;
		}
	}

	return DefWindowProc(wnd, msg, wparam, lparam);
}

void context_open(const char* title, i32 x, i32 y, u32 width, u32 height)
{
	// Init opengl!
	init_opengl();

	// Get module instance
	HINSTANCE instance = GetModuleHandle(NULL);

	// Register window class if we haven't
	if (!class_was_registered)
	{
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = wnd_proc;
		wc.hInstance = instance;
		wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
		wc.lpszClassName = class_name;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.style = CS_OWNDC;

		assert(RegisterClass(&wc));
		class_was_registered = true;
	}

	// Calculate window-size so that the client has the specified size
	u32 wnd_width = width * CELL_WIDTH * window_scale;
	u32 wnd_height = height * CELL_HEIGHT * window_scale;
	client_to_wnd_size(&wnd_width, &wnd_height);

	context.x = x;
	context.y = y;
	context.width = width;
	context.height = height;

	// Open window!
	wnd_handle = CreateWindow(
		class_name,
		title,
		window_style,
		context.x, context.y, wnd_width, wnd_height,
		0, 0,
		instance,
		0
	);

	/** Create GL context **/
	HDC device_context = GetDC(wnd_handle);

	// Fetch pixel format
	i32 pixel_format_attribs[] = {
		WGL_DRAW_TO_WINDOW_ARB,		GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,		GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB,		GL_TRUE,
		WGL_ACCELERATION_ARB,		WGL_FULL_ACCELERATION_ARB,
		WGL_PIXEL_TYPE_ARB,			WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,			32,
		WGL_DEPTH_BITS_ARB,			24,
		WGL_STENCIL_BITS_ARB,		8,
		WGL_SAMPLE_BUFFERS_ARB,		1,
		WGL_SAMPLES_ARB,			0,
		0
	};

	i32 pixel_format;
	UINT num_formats;

	assert(wglChoosePixelFormatARB(device_context, pixel_format_attribs, 0, 1, &pixel_format, &num_formats));
	assert(num_formats);

	// Set that format
	PIXELFORMATDESCRIPTOR pfd;
	DescribePixelFormat(device_context, pixel_format, sizeof(pfd), &pfd);
	assert(SetPixelFormat(device_context, pixel_format, &pfd));

	// Initialize 3.3 context
	i32 context_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB,		3,
		WGL_CONTEXT_MINOR_VERSION_ARB,		3,
		WGL_CONTEXT_PROFILE_MASK_ARB,		WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	HGLRC gl_context = wglCreateContextAttribsARB(device_context, 0, context_attribs);
	assert(gl_context);

	assert(wglMakeCurrent(device_context, gl_context));

	// Disable VSYNC, bro
	wglSwapIntervalEXT(0);

	wnd_context = device_context;
	gl_context = gl_context;

	is_open = true;
}

bool context_is_open()
{
	return is_open;
}

void context_close()
{

}

void context_begin_frame()
{
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void context_end_frame()
{
	SwapBuffers(wnd_context);
	Sleep(1);
}

float time_now()
{
	return 0.f;
}