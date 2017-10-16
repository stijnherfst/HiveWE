#include "stdafx.h"

SLK::SLK(std::string path) {
	load(path);
}

void SLK::load(std::string path) {
	std::ifstream file("Read.txt");
	std::string str;
	while (std::getline(file, str))
	{
		// Process str
	}
}