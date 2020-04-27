#pragma once

typedef struct
{
	u16 width;
	u16 height;
	u8 channels;
	void* data;
} Tga_File;

bool tga_load(Tga_File* tga, const char* path);
void tga_free(Tga_File* tga);

char* file_read_all(const char* path, u32* out_length);