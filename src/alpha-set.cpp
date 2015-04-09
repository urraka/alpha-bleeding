#include <iostream>
#include <fstream>
#include <cstdlib>

#include "png.h"

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		std::cout << "Usage: alpha-set <value> <input> <output>" << std::endl;
		return 0;
	}

	unsigned char value = atoi(argv[1]);
	const char *input = argv[2];
	const char *output = argv[3];

	if (std::ifstream(output).good())
	{
		std::cout << "Output file already exists!" << std::endl;
		return 0;
	}

	int w, h, c;

	unsigned char *data = png_load(input, &w, &h, &c);

	if (data == 0)
	{
		std::cout << "Error loading image. Must be PNG format." << std::endl;
		return 1;
	}

	if (c != 4)
	{
		std::cout << "The image must be 32 bits (RGB with alpha channel)." << std::endl;
		delete[] data;
		return 0;
	}

	const size_t N = 4 * w * h;

	for (size_t i = 3; i < N; i += 4)
		data[i] = value;

	png_save(output, w, h, data);

	delete[] data;

	return 0;
}
