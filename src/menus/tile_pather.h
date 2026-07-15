#pragma once

#include "ui_tile_pather.h"

#include <QWidget>
#include <QDialog>
#include <QButtonGroup>
#include <QAbstractButton>

#include <string>

import FlowLayout;
import Tileset;
import TextureButton;

struct PathingOptions {
	bool unwalkable = false;
	bool unflyable = false;
	bool unbuildable = false;
};

class TilePather: public QDialog {
	Q_OBJECT

  public:
	explicit TilePather(QWidget* parent = nullptr);

  private:
	void changed_tile(QAbstractButton* button);
	void save_tiles();
	uint8_t get_mask(const PathingOptions& options) const;
	void update_selected_icon() const;
	TextureButton* create_tex_button(const TerrainTexture* tex);

	Ui::TilePather ui;
	QButtonGroup* selected_group = new QButtonGroup;
	FlowLayout* selected_layout = new FlowLayout;

	TextureButton* selected_button;
	std::unordered_map<std::string, PathingOptions> pathing_options;
};
