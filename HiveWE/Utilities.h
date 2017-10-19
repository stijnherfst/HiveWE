#pragma once

unsigned char* SOIL_load_image_flipped(const char *filename, int *width, int *height, int *channels, int force_channels);

std::vector<std::string> split(const std::string& string, char delimiter);

//template <typename T>
//std::vector<T> make_unique_and_sort(std::vector<T> vec, const std::function <bool(T, T)>& f) {
//	std::vector<Corner> list = { topLeft, topRight, bottomLeft, bottomRight };
//	std::sort(list.begin(), list.end());
//
//	auto comp = [=](Corner l, Corner r) { return l.ground_texture < r.ground_texture;
//	auto unique_location = std::unique(list.begin(), list.end(), comp);
//	list.resize(std::distance(list.begin(), unique_location));
//}