#pragma once

#include <QOpenGLFunctions_4_5_Core>

#include "BinaryReader.h"

class ShadowMap {
	int width;
	int height;

	GLuint texture;
	std::vector<uint8_t> cells;

	bool load(BinaryReader& reader);
	void save() const;
};