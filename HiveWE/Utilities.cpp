#include "stdafx.h"

unsigned char* SOIL_load_image_flipped(const char *filename, int *width, int *height, int *channels, int force_channels) {
	unsigned char* image = SOIL_load_image(filename, width, height, channels, force_channels);

	int i, j;
	for (j = 0; j * 2 < *height; ++j)
	{
		int index1 = j * *width * *channels;
		int index2 = (*height - 1 - j) * *width * *channels;
		for (i = *width * *channels; i > 0; --i)
		{
			unsigned char temp = image[index1];
			image[index1] = image[index2];
			image[index2] = temp;
			++index1;
			++index2;
		}
	}

	return image;
}

