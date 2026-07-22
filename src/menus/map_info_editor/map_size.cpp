#include "map_info_editor.h"

#include <QMessageBox>
#include <QPainter>

namespace fs = std::filesystem;

void MapInfoEditor::setup_map_size(const Terrain& terrain, const MapInfo& info) {
	old_map_bottom_left.x = 0;
	old_map_bottom_left.y = 0;
	old_map_top_right.x = terrain.width - 1;
	old_map_top_right.y = terrain.height - 1;
	new_map_top_right = old_map_top_right;
	new_map_bottom_left = old_map_bottom_left;

	old_playable_bottom_left.x = info.camera_complements[0];
	old_playable_bottom_left.y = info.camera_complements[2];
	old_playable_top_right.x = old_map_top_right.x - info.camera_complements[1];
	old_playable_top_right.y = old_map_top_right.y - info.camera_complements[3];
	new_playable_bottom_left = old_playable_bottom_left;
	new_playable_top_right = old_playable_top_right;

	terrain_offset = terrain.offset;
	original_minimap = terrain.minimap_image();

	// connect arrow buttons to map size
	connect(ui.mapBoundsLeftDec, &QPushButton::clicked, [this]() {
		adjust_bounds(1, 0, 0, 0);
	});
	connect(ui.mapBoundsLeftInc, &QPushButton::clicked, [this]() {
		adjust_bounds(-1, 0, 0, 0);
	});
	connect(ui.mapBoundsRightDec, &QPushButton::clicked, [this]() {
		adjust_bounds(0, -1, 0, 0);
	});
	connect(ui.mapBoundsRightInc, &QPushButton::clicked, [this]() {
		adjust_bounds(0, 1, 0, 0);
	});
	connect(ui.mapBoundsTopDec, &QPushButton::clicked, [this]() {
		adjust_bounds(0, 0, -1, 0);
	});
	connect(ui.mapBoundsTopInc, &QPushButton::clicked, [this]() {
		adjust_bounds(0, 0, 1, 0);
	});
	connect(ui.mapBoundsBottomDec, &QPushButton::clicked, [this]() {
		adjust_bounds(0, 0, 0, 1);
	});
	connect(ui.mapBoundsBottomInc, &QPushButton::clicked, [this]() {
		adjust_bounds(0, 0, 0, -1);
	});

	// reset camera bounds (unplayable area) to default
	connect(ui.resetCameraBounds, &QPushButton::clicked, [this]() {
		new_playable_bottom_left.x = new_map_bottom_left.x + 6;
		new_playable_bottom_left.y = new_map_bottom_left.y + 4;
		new_playable_top_right.x = new_map_top_right.x - 6;
		new_playable_top_right.y = new_map_top_right.y - 8;
		update_map_size_gui();
	});

	update_map_size_gui();
}

bool MapInfoEditor::save_map_size(Map& map) const {
	const bool changed_map_size = (new_map_bottom_left != old_map_bottom_left) || (new_map_top_right != old_map_top_right);
	const bool changed_playable_size =
		(new_playable_bottom_left != old_playable_bottom_left) || (new_playable_top_right != old_playable_top_right);

	if (changed_map_size || changed_playable_size) {
		const int new_width = new_map_top_right.x - new_map_bottom_left.x;
		const int new_height = new_map_top_right.y - new_map_bottom_left.y;

		const int new_playable_width = new_playable_top_right.x - new_playable_bottom_left.x;
		const int new_playable_height = new_playable_top_right.y - new_playable_bottom_left.y;

		if (new_width < 32 || new_width > 480 || new_height < 32 || new_height > 480) {
			QMessageBox::critical(
				const_cast<MapInfoEditor*>(this),
				"Invalid Map Size",
				QString("Map dimensions must be between 32 and 480.\nNew size would be: %1 x %2").arg(new_width).arg(new_height)
			);
			return false;
		}

		if (new_playable_width < 9 || new_playable_height < 5) {
			QMessageBox::critical(
				const_cast<MapInfoEditor*>(this),
				"Invalid Playable Area",
				QString("Playable area must be at least 9x5.\nNew playable size would be: %1 x %2")
					.arg(new_playable_width)
					.arg(new_playable_height)
			);
			return false;
		}

		// to make this simpler, we first get rid of old boundaries
		if (changed_map_size || changed_playable_size) {
			map.set_playable_area(0, 0, 0, 0);
		}

		// resize the terrain
		if (changed_map_size) {
			const int delta_left = old_map_bottom_left.x - new_map_bottom_left.x;
			const int delta_right = new_map_top_right.x - old_map_top_right.x;
			const int delta_bottom = old_map_bottom_left.y - new_map_bottom_left.y;
			const int delta_top = new_map_top_right.y - old_map_top_right.y;
			map.resize(delta_left, delta_right, delta_top, delta_bottom);
		}

		// apply camera bounds changes
		if (changed_map_size || changed_playable_size) {
			const int unplayable_left = new_playable_bottom_left.x - new_map_bottom_left.x;
			const int unplayable_right = new_map_top_right.x - new_playable_top_right.x;
			const int unplayable_bottom = new_playable_bottom_left.y - new_map_bottom_left.y;
			const int unplayable_top = new_map_top_right.y - new_playable_top_right.y;
			map.set_playable_area(unplayable_left, unplayable_right, unplayable_top, unplayable_bottom);
		}
	}
	return true;
}

void MapInfoEditor::update_map_size_gui() {
	// update text in the menu
	update_bounds_text();

	// update the minimap image
	update_bounds_preview();
}

void MapInfoEditor::update_bounds_text() const {
	int new_width = new_map_top_right.x - new_map_bottom_left.x;
	int new_height = new_map_top_right.y - new_map_bottom_left.y;

	int new_playable_width = new_playable_top_right.x - new_playable_bottom_left.x;
	int new_playable_height = new_playable_top_right.y - new_playable_bottom_left.y;

	const float offset_x = terrain_offset.x + (new_map_bottom_left.x - old_map_bottom_left.x) * 128.f;
	const float offset_y = terrain_offset.y + (new_map_bottom_left.y - old_map_bottom_left.y) * 128.f;

	// update  map size labels
	ui.mapSizeFull->setText(QString::fromStdString(std::format("{} x {}", new_width, new_height)));
	ui.mapSizePlayable->setText(QString::fromStdString(std::format("{} x {}", new_playable_width, new_playable_height)));

	// update map extents and camera bounds text
	ui.mapBoundsLeft->setText(QString::number(offset_x));
	ui.mapBoundsRight->setText(QString::number(offset_x + new_width * 128.f));
	ui.mapBoundsTop->setText(QString::number(offset_y + new_height * 128.f));
	ui.mapBoundsBottom->setText(QString::number(offset_y));

	ui.cameraBoundsLeft->setText(QString::number(offset_x + (new_playable_bottom_left.x + 4 - new_map_bottom_left.x) * 128.f));
	ui.cameraBoundsRight->setText(QString::number(offset_x + (new_playable_top_right.x - 4 - new_map_bottom_left.x) * 128.f));
	ui.cameraBoundsTop->setText(QString::number(offset_y + (new_playable_top_right.y - 2 - new_map_bottom_left.y) * 128.f));
	ui.cameraBoundsBottom->setText(QString::number(offset_y + (new_playable_bottom_left.y + 2 - new_map_bottom_left.y) * 128.f));

	// map size text - determine size based on surface area
	const int surface_area = new_width * new_height;
	QString size_description;

	if (surface_area <= 80 * 80) {
		size_description = "Tiny";
	} else if (surface_area <= 112 * 112) {
		size_description = "Extra Small";
	} else if (surface_area <= 144 * 144) {
		size_description = "Small";
	} else if (surface_area <= 176 * 176) {
		size_description = "Medium";
	} else if (surface_area <= 208 * 208) {
		size_description = "Large";
	} else if (surface_area <= 272 * 272) {
		size_description = "Extra Large";
	} else if (surface_area <= 364 * 364) {
		size_description = "Huge";
	} else {
		size_description = "Epic";
	}

	ui.mapSizeDescription->setText(size_description);
}

void MapInfoEditor::update_bounds_preview() const {
	const int delta_left = old_map_bottom_left.x - new_map_bottom_left.x;
	const int delta_right = new_map_top_right.x - old_map_top_right.x;
	const int delta_bottom = old_map_bottom_left.y - new_map_bottom_left.y;
	const int delta_top = new_map_top_right.y - old_map_top_right.y;

	const int new_width = original_minimap.width + delta_left + delta_right;
	const int new_height = original_minimap.height + delta_bottom + delta_top;

	Texture new_minimap_tex;
	new_minimap_tex.width = new_width;
	new_minimap_tex.height = new_height;
	new_minimap_tex.channels = 4;
	new_minimap_tex.data.resize(new_width * new_height * 4);

	// crate the new image
	for (int y = 0; y < new_height; y++) {
		for (int x = 0; x < new_width; x++) {
			// original coordinates
			const int src_x = x - delta_left;
			const int src_y = y - delta_top;

			const int dst_index = y * new_width * 4 + x * 4;

			// copy from original image if possible, fill with green otherwise
			if (src_x >= 0 && src_x < original_minimap.width && src_y >= 0 && src_y < original_minimap.height) {
				const int src_index = src_y * original_minimap.width * 4 + src_x * 4;
				new_minimap_tex.data[dst_index + 0] = original_minimap.data[src_index + 0];
				new_minimap_tex.data[dst_index + 1] = original_minimap.data[src_index + 1];
				new_minimap_tex.data[dst_index + 2] = original_minimap.data[src_index + 2];
				new_minimap_tex.data[dst_index + 3] = original_minimap.data[src_index + 3];
			} else {
				new_minimap_tex.data[dst_index + 0] = 0;
				new_minimap_tex.data[dst_index + 1] = 192;
				new_minimap_tex.data[dst_index + 2] = 0;
				new_minimap_tex.data[dst_index + 3] = 255;
			}

			// check if pixel is in unplayable area
			const int map_x = new_map_bottom_left.x + x;
			const int map_y = new_map_top_right.y - y;
			const bool is_unplayable =
				(map_x < new_playable_bottom_left.x || map_x > new_playable_top_right.x || map_y < new_playable_bottom_left.y
				 || map_y > new_playable_top_right.y);

			// unplayable pixels are lighter
			if (is_unplayable) {
				new_minimap_tex.data[dst_index + 0] = (new_minimap_tex.data[dst_index + 0] + 255) / 2;
				new_minimap_tex.data[dst_index + 1] = (new_minimap_tex.data[dst_index + 1] + 255) / 2;
				new_minimap_tex.data[dst_index + 2] = (new_minimap_tex.data[dst_index + 2] + 255) / 2;
			}
		}
	}

	// create image with transparent background
	const QImage temp_image = QImage(
		new_minimap_tex.data.data(),
		new_minimap_tex.width,
		new_minimap_tex.height,
		new_minimap_tex.width * new_minimap_tex.channels,
		QImage::Format::Format_RGBA8888
	);
	const QPixmap source_pixmap = QPixmap::fromImage(temp_image);

	// scale the pixmap with sharp pixels (no smoothing)
	QPixmap scaled_pixmap = source_pixmap.scaled(ui.boundsPreview->size(), Qt::KeepAspectRatio, Qt::FastTransformation);

	// calculate camera bounds in original image coordinates
	const int camera_bounds_left = new_playable_bottom_left.x + 4 - new_map_bottom_left.x;
	const int camera_bounds_bottom = new_playable_bottom_left.y + 2 - new_map_bottom_left.y;
	const int camera_bounds_right = new_playable_top_right.x - 4 - new_map_bottom_left.x;
	const int camera_bounds_top = new_playable_top_right.y - 2 - new_map_bottom_left.y;

	// scale coordinates to match the scaled pixmap
	const float scale_x = static_cast<float>(scaled_pixmap.width()) / new_width;
	const float scale_y = static_cast<float>(scaled_pixmap.height()) / new_height;

	const int scaled_left = static_cast<int>(camera_bounds_left * scale_x);
	const int scaled_top = static_cast<int>((new_height - camera_bounds_top - 1) * scale_y);
	const int scaled_right = static_cast<int>(camera_bounds_right * scale_x);
	const int scaled_bottom = static_cast<int>((new_height - camera_bounds_bottom - 1) * scale_y);

	// draw camera bounds rectangle on scaled image
	const QPen pen(QColor(0, 120, 255), 2);
	QPainter painter(&scaled_pixmap);
	painter.setPen(pen);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.drawRect(scaled_left, scaled_top, scaled_right - scaled_left, scaled_bottom - scaled_top);
	painter.end();

	ui.boundsPreview->setPixmap(scaled_pixmap);
}

void MapInfoEditor::adjust_bounds(int delta_left, int delta_right, int delta_top, int delta_bottom) {
	int new_width = new_map_top_right.x - new_map_bottom_left.x;
	int new_height = new_map_top_right.y - new_map_bottom_left.y;

	const int offset_x = static_cast<int>(terrain_offset.x / 128.0f);
	const int offset_y = static_cast<int>(terrain_offset.y / 128.0f);

	// handle terrain size change
	if (ui.modifyMapBounds->isChecked()) {
		// vanilla editor behaviour - changing map is 4 times faster
		delta_left *= 4;
		delta_right *= 4;
		delta_top *= 4;
		delta_bottom *= 4;

		// accept the adjustment if the map is within acceptable bounds
		new_width += delta_left + delta_right;
		new_height += delta_top + delta_bottom;

		// check if the map is not too large, or too small
		const bool allowed_size = new_width >= 32 and new_width <= 480 and new_height >= 32 and new_height <= 480;

		// check if the map extents are valid (vanilla WE constraint)
		const int left_extent = new_map_bottom_left.x + offset_x - delta_left;
		const int right_extent = new_map_top_right.x + offset_x + delta_right;
		const int top_extent = new_map_top_right.y + offset_y + delta_top;
		const int bottom_extent = new_map_bottom_left.y + offset_y - delta_bottom;
		const bool valid_extents = (left_extent >= -252 && right_extent <= 252 && bottom_extent >= -252 && top_extent <= 252);

		// bounds change
		if (allowed_size && valid_extents) {
			new_map_bottom_left.x -= delta_left;
			new_map_bottom_left.y -= delta_bottom;
			new_map_top_right.x += delta_right;
			new_map_top_right.y += delta_top;
		}
	}

	// handle playable area change
	if (ui.modifyCameraBounds->isChecked()) {
		new_playable_bottom_left.x -= delta_left;
		new_playable_bottom_left.y -= delta_bottom;
		new_playable_top_right.x += delta_right;
		new_playable_top_right.y += delta_top;
	}

	// ensure playable area stays within map bounds
	new_playable_bottom_left.x = std::max(new_playable_bottom_left.x, new_map_bottom_left.x);
	new_playable_bottom_left.y = std::max(new_playable_bottom_left.y, new_map_bottom_left.y);
	new_playable_top_right.x = std::min(new_playable_top_right.x, new_map_top_right.x);
	new_playable_top_right.y = std::min(new_playable_top_right.y, new_map_top_right.y);

	// ensure a minimum 9x5 playable area size
	const int playable_width = new_playable_top_right.x - new_playable_bottom_left.x;
	const int playable_height = new_playable_top_right.y - new_playable_bottom_left.y;

	if (playable_width < 9) {
		const int deficit = 9 - playable_width;
		// expand playable area on the opposite side to compensate
		if (delta_left != 0) {
			// left was adjusted, expand right
			new_playable_top_right.x += deficit;
		} else if (delta_right != 0) {
			// right was adjusted, expand left
			new_playable_bottom_left.x -= deficit;
		}
	}

	if (playable_height < 5) {
		const int deficit = 5 - playable_height;
		// expand playable area on the opposite side to compensate
		if (delta_bottom != 0) {
			// bottom was adjusted, expand top
			new_playable_top_right.y += deficit;
		} else if (delta_top != 0) {
			// top was adjusted, expand bottom
			new_playable_bottom_left.y -= deficit;
		}
	}

	update_map_size_gui();
}
