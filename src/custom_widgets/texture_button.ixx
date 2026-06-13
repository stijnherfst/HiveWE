module;

export module TextureButton;

import Tileset;
import <QPushButton>;

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

  private:
	const TerrainTexture* tex;
};
