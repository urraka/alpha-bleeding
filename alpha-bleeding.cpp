#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>
#include <png.h>

void     alpha_bleeding_apply(uint8_t *image, int width, int height);
uint8_t *png_load(const char *path, int *width, int *height, int *channels);
bool     png_save(const char *filename, int width, int height, unsigned char *data);

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cout << "Usage: alpha-bleeding <input> <output>" << std::endl;
		return 0;
	}

	if (std::ifstream(argv[2]).good())
	{
		std::cout << "Output file already exists!" << std::endl;
		return 0;
	}

	int width;
	int height;
	int channels;

	uint8_t *data = png_load(argv[1], &width, &height, &channels);

	if (data == 0)
	{
		std::cout << "Error loading image. Must be PNG format." << std::endl;
		return 1;
	}

	if (channels != 4)
	{
		std::cout << "The image must be 32 bits (RGB with alpha channel)." << std::endl;
		delete[] data;
		return 0;
	}

	alpha_bleeding_apply(data, width, height);

	png_save(argv[2], width, height, data);

	delete[] data;
}

void alpha_bleeding_apply(uint8_t *image, int width, int height)
{
	const size_t N = width * height;

	std::vector<int8_t> opaque(N);
	std::vector<bool>   loose(N);
	std::vector<size_t> pending;
	std::vector<size_t> pendingNext;

	pending.reserve(N);
	pendingNext.reserve(N);

	int offsets[][2] = {
		{-1, -1},
		{ 0, -1},
		{ 1, -1},
		{-1,  0},
		{ 1,  0},
		{-1,  1},
		{ 0,  1},
		{ 1,  1}
	};

	for (size_t i = 0, j = 3; i < N; i++, j += 4)
	{
		if (image[j] == 0)
		{
			bool isLoose = true;

			int x = i % width;
			int y = i / width;

			for (int k = 0; k < 8; k++)
			{
				int s = offsets[k][0];
				int t = offsets[k][1];

				if (x + s > 0 && x + s < width && y + t > 0 && y + t < height)
				{
					size_t index = j + 4 * (s + t * width);

					if (image[index + 3] != 0)
					{
						isLoose = false;
						break;
					}
				}
			}

			if (!isLoose)
				pending.push_back(i);
			else
				loose[i] = true;
		}
		else
		{
			opaque[i] = -1;
		}
	}

	while (pending.size() > 0)
	{
		pendingNext.clear();

		for (size_t p = 0; p < pending.size(); p++)
		{
			size_t i = pending[p] * 4;
			size_t j = pending[p];

			int x = j % width;
			int y = j / width;

			int r = 0;
			int g = 0;
			int b = 0;

			int count = 0;

			for (size_t k = 0; k < 8; k++)
			{
				int s = offsets[k][0];
				int t = offsets[k][1];

				if (x + s > 0 && x + s < width && y + t > 0 && y + t < height)
				{
					t *= width;

					if (opaque[j + s + t] & 1)
					{
						size_t index = i + 4 * (s + t);

						r += image[index + 0];
						g += image[index + 1];
						b += image[index + 2];

						count++;
					}
				}
			}

			if (count > 0)
			{
				image[i + 0] = r / count;
				image[i + 1] = g / count;
				image[i + 2] = b / count;

				opaque[j] = 0xFE;

				for (size_t k = 0; k < 8; k++)
				{
					int s = offsets[k][0];
					int t = offsets[k][1];

					if (x + s > 0 && x + s < width && y + t > 0 && y + t < height)
					{
						size_t index = j + s + t * width;

						if (loose[index])
						{
							pendingNext.push_back(index);
							loose[index] = false;
						}
					}
				}
			}
			else
			{
				pendingNext.push_back(j);
			}
		}

		if (pendingNext.size() > 0)
		{
			for (size_t p = 0; p < pending.size(); p++)
				opaque[pending[p]] >>= 1;
		}

		pending.swap(pendingNext);
	}
}

uint8_t *png_load(const char *path, int *width, int *height, int *channels)
{
	FILE *file = fopen(path, "rb");

	if (!file)
		return 0;

	size_t headerSize = 8;
	uint8_t header[8];
	headerSize = fread(header, 1, headerSize, file);

	if (png_sig_cmp(header, 0, headerSize))
	{
		fclose(file);
		return 0;
	}

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png)
	{
		fclose(file);
		return 0;
	}

	png_infop info = png_create_info_struct(png);

	if (!info)
	{
		png_destroy_read_struct(&png, NULL, NULL);
		fclose(file);
		return 0;
	}

	png_infop info_end = png_create_info_struct(png);

	if (!info_end)
	{
		png_destroy_read_struct(&png, &info, NULL);
		fclose(file);
		return 0;
	}

	if (setjmp(png_jmpbuf(png)))
	{
		png_destroy_read_struct(&png, &info, &info_end);
		fclose(file);
		return 0;
	}

	png_init_io(png, file);
	png_set_sig_bytes(png, headerSize);
	png_read_png(png, info, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, NULL);

	int color = png_get_color_type(png, info);
	*width = png_get_image_width(png, info);
	*height = png_get_image_height(png, info);

	if (color != PNG_COLOR_TYPE_RGB && color != PNG_COLOR_TYPE_RGB_ALPHA)
	{
		png_destroy_read_struct(&png, &info, &info_end);
		fclose(file);
		return 0;
	}

	png_bytep *rows = png_get_rows(png, info);

	*channels = (color == PNG_COLOR_TYPE_RGB_ALPHA ? 4 : 3);

	uint8_t *image = new uint8_t[(*width) * (*height) * (*channels)];

	for (int y = 0; y < *height; y++)
		memcpy(image + y * (*width) * (*channels), rows[y], (*width) * (*channels));

	png_destroy_read_struct(&png, &info, &info_end);
	fclose(file);

	return image;
}

bool png_save(const char *filename, int width, int height, unsigned char *data)
{
	FILE *png_file = fopen(filename, "wb");

	if (!png_file)
		return false;

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
		return false;

	png_infop info_ptr = png_create_info_struct(png_ptr);

	if (!info_ptr)
		return false;

	png_init_io(png_ptr, png_file);

	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_byte **row_ptrs = new png_byte*[height];

	for (int i = 0; i < height; i++)
		row_ptrs[i] = data + i * width * 4;

	png_set_rows(png_ptr, info_ptr, row_ptrs);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(png_file);

	delete[] row_ptrs;

	return true;
}
