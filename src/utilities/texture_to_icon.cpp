#include "texture_to_icon.h"

#include <QImage>
#include <QPixmap>

import ResourceManager;
import Texture;

QIcon texture_to_icon(const std::filesystem::path& path) {
    const auto tex = resource_manager.load<Texture>(path).value();
    const QImage temp_image(
        tex->data.data(),
        tex->width,
        tex->height,
        tex->channels == 3
            ? QImage::Format::Format_RGB888
            : QImage::Format::Format_RGBA8888
    );

    return QIcon(QPixmap::fromImage(temp_image));
}
