#include "import.h"
#include <stdio.h>
#include <stdlib.h>

const u8 RUN_LENGTH_BIT = 0b1000;

enum Tga_Image_Type
{
	TGA_No_Image_Data,
	TGA_Color_Mapped,
	TGA_True_Color,
	TGA_Black_And_White
};

#pragma pack(push, 1)

typedef struct
{
	// Length of image id field
	u8 id_length;

	// 0 if no color map, 1 if there is
	u8 color_map_type;

	// Describes the image type (colored, grayscale etc.)
	u8 image_type;
} Tga_Header;

typedef struct
{
	u16 first_index;
	u16 length;
	u8 bits_per_pixel;
} Tga_Color_Map;

typedef struct
{
	u16 x_origin;
	u16 y_origin;
	u16 width;
	u16 height;
	u8 pixel_depth;
	u8 image_descriptor;
} Tga_Image_Spec;

#pragma pack(pop)

bool tga_load(Tga_File* tga, const char* path)
{
	FILE* file = fopen(path, "rb");
	if (file == NULL)
	{
		msg_box("Failed to load TGA file '%s', file doesn't exist", path);
		return false;
	}

	// Read header
	Tga_Header header;
	Tga_Color_Map color_map;
	Tga_Image_Spec image_spec;

	fread(&header, sizeof(header), 1, file);
	fread(&color_map, sizeof(color_map), 1, file);
	fread(&image_spec, sizeof(image_spec), 1, file);

	// Read image ID
	fread(NULL, header.id_length, 1, file);

	// Read pixel data
	tga->width = image_spec.width;
	tga->height = image_spec.height;
	tga->channels = image_spec.pixel_depth / 8;

	u32 image_size = image_spec.width * image_spec.height * tga->channels;
	tga->data = malloc(image_size);
	fread(tga->data, image_size, 1, file);

	fclose(file);
	return true;
}

void tga_free(Tga_File* tga)
{
	if (tga->data != NULL)
	{
		free(tga->data);
		tga->data = NULL;
	}

	tga->width = 0;
	tga->height = 0;
}

char* file_read_all(const char* path, u32* out_length)
{
	FILE* file = fopen(path, "rb");
	if (file == NULL)
	{
		msg_box("Failed to read file '%s', file doesn't exist", path);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	u32 file_len = ftell(file);
	if (out_length != NULL)
		*out_length = file_len;

	fseek(file, 0, SEEK_SET);

	char* buffer = malloc(file_len);
	fread(buffer, 1, file_len, file);

	fclose(file);
	return buffer;
}