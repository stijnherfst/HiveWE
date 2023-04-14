module;

#include <filesystem>
#include <vector>
#include <QIcon>
#include <QImage>
#include <QPixMap>

export module QIconResource;

import ResourceManager;
import Texture;

namespace fs = std::filesystem;

export class QIconResource : public Resource {
  public:
	QIcon icon;

	static constexpr const char* name = "QIconResource";

	explicit QIconResource() = default;

	explicit QIconResource(const fs::path& path) {
		const auto image = resource_manager.load<Texture>(path);
		QImage temp_image(image->data.data(), image->width, image->height, image->channels == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_RGBA8888);
		auto pix = QPixmap::fromImage(temp_image);
		icon = QIcon(pix);
	}
};