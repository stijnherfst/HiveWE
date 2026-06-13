#pragma once

#include <vector>
#include <QButtonGroup>
#include <QDialog>
#include "ui_tile_setter.h"

#include <vector>

import FlowLayout;
import Tileset;
import TextureButton;

class TileSetter: public QDialog {
	Q_OBJECT

  public:
	explicit TileSetter(QWidget* parent = nullptr);

  private:
	void add_tile();
	void remove_tile() const;
	void update_available_tiles();
	void shift_left() const;
	void shift_right() const;
	void save_tiles();
	void reset_to_default();
	void update_gui() const;
	void set_scroll_view_height(QScrollArea* scroll_area, const FlowLayout* layout) const;
	TextureButton* create_tex_button(const TerrainTexture* tex);

	Ui::TileSetter ui;

	// QListWidget* widget_list = new QListWidget;

	QButtonGroup* selected_group = new QButtonGroup;
	QButtonGroup* available_group = new QButtonGroup;

	FlowLayout* selected_layout = new FlowLayout;
	FlowLayout* available_layout = new FlowLayout;

	std::vector<int> from_to_id;
};
