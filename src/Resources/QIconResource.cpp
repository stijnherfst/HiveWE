#include "QIconResource.h"

#include <soil2/SOIL2.h>
#include <QImage>
#include <QPixmap>

import Hierarchy;
import BLP;
import BinaryReader;
import Texture;

QIconResource::QIconResource(const fs::path& path) {
	const auto image = resource_manager.load<Texture>(path);
	QImage temp_image(image->data.data(), image->width, image->height, image->channels == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_RGBA8888);
	auto pix = QPixmap::fromImage(temp_image);
	icon = QIcon(pix);
}