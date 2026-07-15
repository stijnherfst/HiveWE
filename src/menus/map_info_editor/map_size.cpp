#include "map_info_editor.h"

#include <QMessageBox>
#include <QPainter>

import MapGlobal;

namespace fs = std::filesystem;

void MapInfoEditor::setup_map_size() {
	oldMapBottomLeft.x = 0;
	oldMapBottomLeft.y = 0;
	oldMapTopRight.x = map->terrain.width - 1;
	oldMapTopRight.y = map->terrain.height - 1;
	newMapTopRight = oldMapTopRight;
	newMapBottomLeft = oldMapBottomLeft;

	oldPlayableBottomLeft.x = map->info.camera_complements[0];
	oldPlayableBottomLeft.y = map->info.camera_complements[2];
	oldPlayableTopRight.x = oldMapTopRight.x - map->info.camera_complements[1];
	oldPlayableTopRight.y = oldMapTopRight.y - map->info.camera_complements[3];
	newPlayableBottomLeft = oldPlayableBottomLeft;
	newPlayableTopRight = oldPlayableTopRight;

	originalMinimap = map->terrain.minimap_image();

	// connect arrow buttons to map size
	connect(ui.mapBoundsLeftDec, &QPushButton::clicked, [this]() {
		adjustBounds(1, 0, 0, 0);
	});
	connect(ui.mapBoundsLeftInc, &QPushButton::clicked, [this]() {
		adjustBounds(-1, 0, 0, 0);
	});
	connect(ui.mapBoundsRightDec, &QPushButton::clicked, [this]() {
		adjustBounds(0, -1, 0, 0);
	});
	connect(ui.mapBoundsRightInc, &QPushButton::clicked, [this]() {
		adjustBounds(0, 1, 0, 0);
	});
	connect(ui.mapBoundsTopDec, &QPushButton::clicked, [this]() {
		adjustBounds(0, 0, -1, 0);
	});
	connect(ui.mapBoundsTopInc, &QPushButton::clicked, [this]() {
		adjustBounds(0, 0, 1, 0);
	});
	connect(ui.mapBoundsBottomDec, &QPushButton::clicked, [this]() {
		adjustBounds(0, 0, 0, 1);
	});
	connect(ui.mapBoundsBottomInc, &QPushButton::clicked, [this]() {
		adjustBounds(0, 0, 0, -1);
	});

	// reset camera bounds (unplayable area) to default
	connect(ui.resetCameraBounds, &QPushButton::clicked, [this]() {
		newPlayableBottomLeft.x = newMapBottomLeft.x + 6;
		newPlayableBottomLeft.y = newMapBottomLeft.y + 4;
		newPlayableTopRight.x = newMapTopRight.x - 6;
		newPlayableTopRight.y = newMapTopRight.y - 8;
		updateMapSizeGUI();
	});

	updateMapSizeGUI();
}

bool MapInfoEditor::save_map_size() const {
	const bool changedMapSize = (newMapBottomLeft != oldMapBottomLeft) || (newMapTopRight != oldMapTopRight);
	const bool changedPlayableSize = (newPlayableBottomLeft != oldPlayableBottomLeft) || (newPlayableTopRight != oldPlayableTopRight);

	if (changedMapSize || changedPlayableSize) {
		const int newWidth = newMapTopRight.x - newMapBottomLeft.x;
		const int newHeight = newMapTopRight.y - newMapBottomLeft.y;

		const int newPlayableWidth = newPlayableTopRight.x - newPlayableBottomLeft.x;
		const int newPlayableHeight = newPlayableTopRight.y - newPlayableBottomLeft.y;

		if (newWidth < 32 || newWidth > 480 || newHeight < 32 || newHeight > 480) {
			QMessageBox::critical(
				const_cast<MapInfoEditor*>(this),
				"Invalid Map Size",
				QString("Map dimensions must be between 32 and 480.\nNew size would be: %1 x %2").arg(newWidth).arg(newHeight)
			);
			return false;
		}

		if (newPlayableWidth < 9 || newPlayableHeight < 5) {
			QMessageBox::critical(
				const_cast<MapInfoEditor*>(this),
				"Invalid Playable Area",
				QString("Playable area must be at least 9x5.\nNew playable size would be: %1 x %2")
					.arg(newPlayableWidth)
					.arg(newPlayableHeight)
			);
			return false;
		}

		// to make this simpler, we first get rid of old boundaries
		if (changedMapSize || changedPlayableSize) {
			map->set_playable_area(0, 0, 0, 0);
		}

		// resize the terrain
		if (changedMapSize) {
			const int deltaLeft = oldMapBottomLeft.x - newMapBottomLeft.x;
			const int deltaRight = newMapTopRight.x - oldMapTopRight.x;
			const int deltaBottom = oldMapBottomLeft.y - newMapBottomLeft.y;
			const int deltaTop = newMapTopRight.y - oldMapTopRight.y;
			map->resize(deltaLeft, deltaRight, deltaTop, deltaBottom);
		}

		// apply camera bounds changes
		if (changedMapSize || changedPlayableSize) {
			const int unplayableLeft = newPlayableBottomLeft.x - newMapBottomLeft.x;
			const int unplayableRight = newMapTopRight.x - newPlayableTopRight.x;
			const int unplayableBottom = newPlayableBottomLeft.y - newMapBottomLeft.y;
			const int unplayableTop = newMapTopRight.y - newPlayableTopRight.y;
			map->set_playable_area(unplayableLeft, unplayableRight, unplayableTop, unplayableBottom);
		}
	}
	return true;
}

void MapInfoEditor::updateMapSizeGUI() {
	// update text in the menu
	updateBoundsText();

	// update the minimap image
	updateBoundsPreview();
}

void MapInfoEditor::updateBoundsText() const {
	int newWidth = newMapTopRight.x - newMapBottomLeft.x;
	int newHeight = newMapTopRight.y - newMapBottomLeft.y;

	int newPlayableWidth = newPlayableTopRight.x - newPlayableBottomLeft.x;
	int newPlayableHeight = newPlayableTopRight.y - newPlayableBottomLeft.y;

	const float offsetX = map->terrain.offset.x + (newMapBottomLeft.x - oldMapBottomLeft.x) * 128.f;
	const float offsetY = map->terrain.offset.y + (newMapBottomLeft.y - oldMapBottomLeft.y) * 128.f;

	// update  map size labels
	ui.mapSizeFull->setText(QString::fromStdString(std::format("{} x {}", newWidth, newHeight)));
	ui.mapSizePlayable->setText(QString::fromStdString(std::format("{} x {}", newPlayableWidth, newPlayableHeight)));

	// update map extents and camera bounds text
	ui.mapBoundsLeft->setText(QString::number(offsetX));
	ui.mapBoundsRight->setText(QString::number(offsetX + newWidth * 128.f));
	ui.mapBoundsTop->setText(QString::number(offsetY + newHeight * 128.f));
	ui.mapBoundsBottom->setText(QString::number(offsetY));

	ui.cameraBoundsLeft->setText(QString::number(offsetX + (newPlayableBottomLeft.x + 4 - newMapBottomLeft.x) * 128.f));
	ui.cameraBoundsRight->setText(QString::number(offsetX + (newPlayableTopRight.x - 4 - newMapBottomLeft.x) * 128.f));
	ui.cameraBoundsTop->setText(QString::number(offsetY + (newPlayableTopRight.y - 2 - newMapBottomLeft.y) * 128.f));
	ui.cameraBoundsBottom->setText(QString::number(offsetY + (newPlayableBottomLeft.y + 2 - newMapBottomLeft.y) * 128.f));

	// map size text - determine size based on surface area
	const int surfaceArea = newWidth * newHeight;
	QString sizeDescription;

	if (surfaceArea <= 80 * 80) {
		sizeDescription = "Tiny";
	} else if (surfaceArea <= 112 * 112) {
		sizeDescription = "Extra Small";
	} else if (surfaceArea <= 144 * 144) {
		sizeDescription = "Small";
	} else if (surfaceArea <= 176 * 176) {
		sizeDescription = "Medium";
	} else if (surfaceArea <= 208 * 208) {
		sizeDescription = "Large";
	} else if (surfaceArea <= 272 * 272) {
		sizeDescription = "Extra Large";
	} else if (surfaceArea <= 364 * 364) {
		sizeDescription = "Huge";
	} else {
		sizeDescription = "Epic";
	}

	ui.mapSizeDescription->setText(sizeDescription);
}

void MapInfoEditor::updateBoundsPreview() const {
	const int deltaLeft = oldMapBottomLeft.x - newMapBottomLeft.x;
	const int deltaRight = newMapTopRight.x - oldMapTopRight.x;
	const int deltaBottom = oldMapBottomLeft.y - newMapBottomLeft.y;
	const int deltaTop = newMapTopRight.y - oldMapTopRight.y;

	const int newWidth = originalMinimap.width + deltaLeft + deltaRight;
	const int newHeight = originalMinimap.height + deltaBottom + deltaTop;

	Texture newMinimapTex;
	newMinimapTex.width = newWidth;
	newMinimapTex.height = newHeight;
	newMinimapTex.channels = 4;
	newMinimapTex.data.resize(newWidth * newHeight * 4);

	// crate the new image
	for (int y = 0; y < newHeight; y++) {
		for (int x = 0; x < newWidth; x++) {
			// original coordinates
			const int srcX = x - deltaLeft;
			const int srcY = y - deltaTop;

			const int dstIndex = y * newWidth * 4 + x * 4;

			// copy from original image if possible, fill with green otherwise
			if (srcX >= 0 && srcX < originalMinimap.width && srcY >= 0 && srcY < originalMinimap.height) {
				const int srcIndex = srcY * originalMinimap.width * 4 + srcX * 4;
				newMinimapTex.data[dstIndex + 0] = originalMinimap.data[srcIndex + 0];
				newMinimapTex.data[dstIndex + 1] = originalMinimap.data[srcIndex + 1];
				newMinimapTex.data[dstIndex + 2] = originalMinimap.data[srcIndex + 2];
				newMinimapTex.data[dstIndex + 3] = originalMinimap.data[srcIndex + 3];
			} else {
				newMinimapTex.data[dstIndex + 0] = 0;
				newMinimapTex.data[dstIndex + 1] = 192;
				newMinimapTex.data[dstIndex + 2] = 0;
				newMinimapTex.data[dstIndex + 3] = 255;
			}

			// check if pixel is in unplayable area
			const int mapX = newMapBottomLeft.x + x;
			const int mapY = newMapTopRight.y - y;
			const bool isUnplayable =
				(mapX < newPlayableBottomLeft.x || mapX > newPlayableTopRight.x || mapY < newPlayableBottomLeft.y
				 || mapY > newPlayableTopRight.y);

			// unplayable pixels are lighter
			if (isUnplayable) {
				newMinimapTex.data[dstIndex + 0] = (newMinimapTex.data[dstIndex + 0] + 255) / 2;
				newMinimapTex.data[dstIndex + 1] = (newMinimapTex.data[dstIndex + 1] + 255) / 2;
				newMinimapTex.data[dstIndex + 2] = (newMinimapTex.data[dstIndex + 2] + 255) / 2;
			}
		}
	}

	// create image with transparent background
	const QImage temp_image = QImage(
		newMinimapTex.data.data(),
		newMinimapTex.width,
		newMinimapTex.height,
		newMinimapTex.width * newMinimapTex.channels,
		QImage::Format::Format_RGBA8888
	);
	const QPixmap sourcePixmap = QPixmap::fromImage(temp_image);

	// scale the pixmap with sharp pixels (no smoothing)
	QPixmap scaledPixmap = sourcePixmap.scaled(ui.boundsPreview->size(), Qt::KeepAspectRatio, Qt::FastTransformation);

	// calculate camera bounds in original image coordinates
	const int cameraBoundsLeft = newPlayableBottomLeft.x + 4 - newMapBottomLeft.x;
	const int cameraBoundsBottom = newPlayableBottomLeft.y + 2 - newMapBottomLeft.y;
	const int cameraBoundsRight = newPlayableTopRight.x - 4 - newMapBottomLeft.x;
	const int cameraBoundsTop = newPlayableTopRight.y - 2 - newMapBottomLeft.y;

	// scale coordinates to match the scaled pixmap
	const float scaleX = static_cast<float>(scaledPixmap.width()) / newWidth;
	const float scaleY = static_cast<float>(scaledPixmap.height()) / newHeight;

	const int scaledLeft = static_cast<int>(cameraBoundsLeft * scaleX);
	const int scaledTop = static_cast<int>((newHeight - cameraBoundsTop - 1) * scaleY);
	const int scaledRight = static_cast<int>(cameraBoundsRight * scaleX);
	const int scaledBottom = static_cast<int>((newHeight - cameraBoundsBottom - 1) * scaleY);

	// draw camera bounds rectangle on scaled image
	const QPen pen(QColor(0, 120, 255), 2);
	QPainter painter(&scaledPixmap);
	painter.setPen(pen);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.drawRect(scaledLeft, scaledTop, scaledRight - scaledLeft, scaledBottom - scaledTop);
	painter.end();

	ui.boundsPreview->setPixmap(scaledPixmap);
}

void MapInfoEditor::adjustBounds(int deltaLeft, int deltaRight, int deltaTop, int deltaBottom) {
	int newWidth = newMapTopRight.x - newMapBottomLeft.x;
	int newHeight = newMapTopRight.y - newMapBottomLeft.y;

	const int offsetX = static_cast<int>(map->terrain.offset.x / 128.0f);
	const int offsetY = static_cast<int>(map->terrain.offset.y / 128.0f);

	// handle terrain size change
	if (ui.modifyMapBounds->isChecked()) {
		// vanilla editor behaviour - changing map is 4 times faster
		deltaLeft *= 4;
		deltaRight *= 4;
		deltaTop *= 4;
		deltaBottom *= 4;

		// accept the adjustment if the map is within acceptable bounds
		newWidth += deltaLeft + deltaRight;
		newHeight += deltaTop + deltaBottom;

		// check if the map is not too large, or too small
		const bool allowedSize = newWidth >= 32 and newWidth <= 480 and newHeight >= 32 and newHeight <= 480;

		// check if the map extents are valid (vanilla WE constraint)
		const int leftExtent = newMapBottomLeft.x + offsetX - deltaLeft;
		const int rightExtent = newMapTopRight.x + offsetX + deltaRight;
		const int topExtent = newMapTopRight.y + offsetY + deltaTop;
		const int bottomExtent = newMapBottomLeft.y + offsetY - deltaBottom;
		const bool validExtents = (leftExtent >= -252 && rightExtent <= 252 && bottomExtent >= -252 && topExtent <= 252);

		// bounds change
		if (allowedSize && validExtents) {
			newMapBottomLeft.x -= deltaLeft;
			newMapBottomLeft.y -= deltaBottom;
			newMapTopRight.x += deltaRight;
			newMapTopRight.y += deltaTop;
		}
	}

	// handle playable area change
	if (ui.modifyCameraBounds->isChecked()) {
		newPlayableBottomLeft.x -= deltaLeft;
		newPlayableBottomLeft.y -= deltaBottom;
		newPlayableTopRight.x += deltaRight;
		newPlayableTopRight.y += deltaTop;
	}

	// ensure playable area stays within map bounds
	newPlayableBottomLeft.x = std::max(newPlayableBottomLeft.x, newMapBottomLeft.x);
	newPlayableBottomLeft.y = std::max(newPlayableBottomLeft.y, newMapBottomLeft.y);
	newPlayableTopRight.x = std::min(newPlayableTopRight.x, newMapTopRight.x);
	newPlayableTopRight.y = std::min(newPlayableTopRight.y, newMapTopRight.y);

	// ensure a minimum 9x5 playable area size
	const int playableWidth = newPlayableTopRight.x - newPlayableBottomLeft.x;
	const int playableHeight = newPlayableTopRight.y - newPlayableBottomLeft.y;

	if (playableWidth < 9) {
		const int deficit = 9 - playableWidth;
		// expand playable area on the opposite side to compensate
		if (deltaLeft != 0) {
			// left was adjusted, expand right
			newPlayableTopRight.x += deficit;
		} else if (deltaRight != 0) {
			// right was adjusted, expand left
			newPlayableBottomLeft.x -= deficit;
		}
	}

	if (playableHeight < 5) {
		const int deficit = 5 - playableHeight;
		// expand playable area on the opposite side to compensate
		if (deltaBottom != 0) {
			// bottom was adjusted, expand top
			newPlayableTopRight.y += deficit;
		} else if (deltaTop != 0) {
			// top was adjusted, expand bottom
			newPlayableBottomLeft.y -= deficit;
		}
	}

	updateMapSizeGUI();
}
