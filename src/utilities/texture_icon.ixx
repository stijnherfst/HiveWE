module;

#include <QPainter>
#include <QIcon>

export module TextureIcon;

import std;
import ResourceManager;
import Texture;

namespace fs = std::filesystem;

/// Loads a texture from the hierarchy and returns an icon
export QIcon texture_to_icon(const fs::path& path) {
	const auto tex = resource_manager.load<Texture>(path).value();
	const QImage temp_image = QImage(
		tex->data.data(),
		tex->width,
		tex->height,
		tex->channels == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_RGBA8888
	);
	const auto pix = QPixmap::fromImage(temp_image);
	return QIcon(pix);
}
