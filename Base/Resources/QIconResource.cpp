#include "QIconResource.h"
#include "Texture.h"

#include <SOIL2/SOIL2.h>
#include <QImage>
#include <QPixmap>

#include "BinaryReader.h"
#include "Hierarchy.h"
#include "BLP.h"

QIconResource::QIconResource(const fs::path& path) {
	const auto image = resource_manager.load<Texture>(path);
	QImage temp_image(image->data.data(), image->width, image->height, image->channels == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_RGBA8888);
	auto pix = QPixmap::fromImage(temp_image);
	icon = QIcon(pix);
}