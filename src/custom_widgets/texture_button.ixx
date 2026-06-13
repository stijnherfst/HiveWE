module;

export module TextureButton;

import Tileset;
import Texture;
import ResourceManager;
import OpenGLUtilities;
import PathingMap;
import <QPushButton>;
import <QPainter>;

export class TextureButton: public QPushButton {
  public:
	explicit TextureButton(const TerrainTexture* t, QWidget* parent = nullptr) : QPushButton(parent), tex(t) {}

	[[nodiscard]]
	const TerrainTexture* texture() const {
		return tex;
	}

	[[nodiscard]]
	bool hasTexture() const {
		return tex != nullptr;
	}

	void create_icon(bool pathing_hint, bool cliff_hint, std::optional<uint8_t> pathing_override = std::nullopt) {
		if (!hasTexture()) {
			return;
		}

		const auto image = resource_manager.load<Texture>(tex->file_path).value();
		const QImage temp_image = QImage(image->data.data(), image->width, image->height, QImage::Format::Format_RGBA8888);

		// take the first "variation" from the ground tile
		const int size = image->height / 4;
		QIcon icon;

		// regular icon
		auto pix = QPixmap::fromImage(temp_image.copy(0, 0, size, size));

		// add hints - the coloured corners depicting terrain texture's pathing
		// or if the terrain texture has a matching cliff texture
		if (pathing_hint) {
			const uint8_t pathing = pathing_override.value_or(tex->get_tile_pathing());
			const bool unwalkable = pathing & PathingMap::Flags::unwalkable;
			const bool unbuildable = pathing & PathingMap::Flags::unbuildable;
			const bool unflyable = pathing & PathingMap::Flags::unflyable;

			if (unwalkable || unbuildable || unflyable) {
				const QColor color(unwalkable ? 255 : 0, unflyable ? 255 : 0, unbuildable ? 255 : 0, 128);

				QPainter painter(&pix);
				painter.fillRect(size / 2, 0, size / 2, size / 2, color);
				painter.end();
			}
		}

		if (cliff_hint && tex->cliff_type_id) {
			const QColor color(255, 0, 0, 128);

			QPainter painter(&pix);
			painter.fillRect(0, 0, size / 2, size / 2, color);
			painter.end();
		}

		icon.addPixmap(pix, QIcon::Normal, QIcon::Off);

		// when pressed, terrain texture icon should get a yellow tint
		QPainter painter(&pix);
		painter.fillRect(0, 0, size, size, QColor(255, 255, 0, 64));
		painter.end();
		icon.addPixmap(pix, QIcon::Normal, QIcon::On);

		setIcon(icon);
	}

  private:
	const TerrainTexture* tex;
};

export class CliffButton: public TextureButton {
  public:
	explicit CliffButton(const CliffType* cliff_type, const TerrainTexture* terr_tex, QWidget* parent = nullptr) :
		TextureButton(terr_tex, parent),
		clf(cliff_type) {}

	[[nodiscard]]
	const CliffType* cliff() const {
		return clf;
	}

	[[nodiscard]]
	bool hasCliff() const {
		return clf != nullptr;
	}

  private:
	const CliffType* clf;
};
