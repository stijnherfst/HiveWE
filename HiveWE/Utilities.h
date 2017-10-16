#pragma once

unsigned char* SOIL_load_image_flipped(const char *filename, int *width, int *height, int *channels, int force_channels);

std::vector<std::string> split(const std::string& string, char delimiter);