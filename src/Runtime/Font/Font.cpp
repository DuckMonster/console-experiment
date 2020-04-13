#include "Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H

bool freetype_valid = false;
FT_Library ft_lib;

void freetype_init()
{
	if (freetype_valid)
		return;

	FT_Init_FreeType(&ft_lib);
	freetype_valid = true;
}

void font_load(Font* font, const char* path)
{
	freetype_init();
	FT_Error error;

	// Load face
	FT_Face face;
	error = FT_New_Face(ft_lib, path, 0, &face);
	FT_Set_Pixel_Sizes(face, 0, 16);

	// Load a single glyph my friend
	u32 glyph_index = FT_Get_Char_Index(face, 'A');
	FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
}