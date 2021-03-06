#pragma once
#include "winmin.h"
#include <GL/GL.h>
#include <GL/wglext.h>
#include <GL/glext.h>

#ifdef IMPLEMENT_GL
void gl_dummy_function()
{
	error("Called OpenGL function before init_opengl was called");
}

// Define this macro if the .c is IMPLEMENTING GL-functions (gl_bind.c)
#define DECL_FUN(fn_ptr, name)\
typedef fn_ptr fn_ ## name;\
fn_ ## name name = (fn_ptr)gl_dummy_function

#else

// Otherwise, extern all the functions
#define DECL_FUN(fn_ptr, name)\
typedef fn_ptr fn_ ## name;\
extern fn_ ## name name

#endif

/******* GL functions ******/
// Vertex-buffers and stuff
DECL_FUN(PFNGLGENVERTEXARRAYSPROC,	glGenVertexArrays);
DECL_FUN(PFNGLDELETEVERTEXARRAYSPROC,	glDeleteVertexArrays);
DECL_FUN(PFNGLBINDVERTEXARRAYPROC,	glBindVertexArray);
DECL_FUN(PFNGLGENBUFFERSPROC,		glGenBuffers);
DECL_FUN(PFNGLDELETEBUFFERSPROC,	glDeleteBuffers);
DECL_FUN(PFNGLBUFFERDATAPROC,		glBufferData);
DECL_FUN(PFNGLBUFFERSUBDATAPROC,	glBufferSubData);
DECL_FUN(PFNGLBINDBUFFERPROC,		glBindBuffer);

DECL_FUN(PFNGLGETATTRIBLOCATIONPROC,glGetAttribLocation);
DECL_FUN(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
DECL_FUN(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
DECL_FUN(PFNGLVERTEXATTRIBIPOINTERPROC, glVertexAttribIPointer);
DECL_FUN(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor);
DECL_FUN(PFNGLVERTEXBINDINGDIVISORPROC, glVertexBindingDivisor);

// Textures
DECL_FUN(PFNGLACTIVETEXTUREPROC,	glActiveTexture);

// Shaders and programs
DECL_FUN(PFNGLCREATESHADERPROC,		glCreateShader);
DECL_FUN(PFNGLDELETESHADERPROC,		glDeleteShader);
DECL_FUN(PFNGLSHADERSOURCEPROC,		glShaderSource);
DECL_FUN(PFNGLGETSHADERSOURCEPROC,	glGetShaderSource);
DECL_FUN(PFNGLCOMPILESHADERPROC,	glCompileShader);
DECL_FUN(PFNGLATTACHSHADERPROC,		glAttachShader);
DECL_FUN(PFNGLDETACHSHADERPROC,		glDetachShader);
DECL_FUN(PFNGLGETSHADERIVPROC,		glGetShaderiv);
DECL_FUN(PFNGLGETSHADERINFOLOGPROC,	glGetShaderInfoLog);
DECL_FUN(PFNGLCREATEPROGRAMPROC,	glCreateProgram);
DECL_FUN(PFNGLDELETEPROGRAMPROC,	glDeleteProgram);
DECL_FUN(PFNGLLINKPROGRAMPROC,		glLinkProgram);
DECL_FUN(PFNGLUSEPROGRAMPROC,		glUseProgram);
DECL_FUN(PFNGLGETPROGRAMIVPROC,		glGetProgramiv);
DECL_FUN(PFNGLGETPROGRAMINFOLOGPROC,glGetProgramInfoLog);

// Uniforms!
DECL_FUN(PFNGLGETUNIFORMLOCATIONPROC,	glGetUniformLocation);
DECL_FUN(PFNGLUNIFORM1IPROC,			glUniform1i);
DECL_FUN(PFNGLUNIFORM1FPROC,			glUniform1f);
DECL_FUN(PFNGLUNIFORM2IPROC,			glUniform2i);
DECL_FUN(PFNGLUNIFORM2FPROC,			glUniform2f);
DECL_FUN(PFNGLUNIFORM2FVPROC,			glUniform2fv);
DECL_FUN(PFNGLUNIFORM3FVPROC,			glUniform3fv);
DECL_FUN(PFNGLUNIFORM4FVPROC,			glUniform4fv);
DECL_FUN(PFNGLUNIFORMMATRIX4FVPROC,		glUniformMatrix4fv);

// Frame buffers
DECL_FUN(PFNGLCREATEFRAMEBUFFERSPROC,	glCreateFramebuffers);
DECL_FUN(PFNGLBINDFRAMEBUFFERPROC,		glBindFramebuffer);
DECL_FUN(PFNGLDELETEFRAMEBUFFERSPROC,	glDeleteFramebuffers);
DECL_FUN(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D);
DECL_FUN(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus);

// Drawing
DECL_FUN(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced);

/******* WGL Extension Functions *******/
DECL_FUN(PFNWGLCHOOSEPIXELFORMATARBPROC,	wglChoosePixelFormatARB);
DECL_FUN(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB);
DECL_FUN(PFNWGLSWAPINTERVALEXTPROC,			wglSwapIntervalEXT);

// Call this to initialize opengl extensions!
void init_opengl();	