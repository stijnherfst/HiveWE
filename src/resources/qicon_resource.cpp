module;

#include <filesystem>

#include <memory>
#include <QIcon>
#include <QImage>
#include <QPixmap>

module QIconResource;

import ResourceManager;
import Texture;

struct QIconResource::Impl {
    QIcon icon;
};

QIconResource::QIconResource()
    : impl(std::make_shared<Impl>()) {}

QIconResource::QIconResource(const fs::path& path)
    : impl(std::make_shared<Impl>()) {
    const auto image = resource_manager.load<Texture>(path).value();

    const QImage temp_image(
        image->data.data(),
        image->width,
        image->height,
        image->channels == 3
            ? QImage::Format::Format_RGB888
            : QImage::Format::Format_RGBA8888
    );

    impl->icon = QIcon(QPixmap::fromImage(temp_image));
}

const QIcon& QIconResource::icon() const {
    return impl->icon;
}
