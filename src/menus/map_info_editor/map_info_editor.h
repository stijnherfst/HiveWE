#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ui_map_info_editor.h"

import Texture;

class MapInfoEditor: public QDialog {
	Q_OBJECT

  public:
	MapInfoEditor(QWidget* parent = nullptr);

	Ui::MapInfoEditor ui;

	bool save() const;

  private:
	void setup_description();
	bool save_description() const;

	void setup_loading_screen();
	bool save_loading_screen() const;

	void setup_options();
	bool save_options() const;

	void setup_map_size();
	bool save_map_size() const;
	void updateMapSizeGUI();
	void adjustBounds(int deltaLeft, int deltaRight, int deltaTop, int deltaBottom);
	void updateBoundsPreview() const;
	void updateBoundsText() const;

	// used for changing map size
	glm::ivec2 oldMapBottomLeft;
	glm::ivec2 oldMapTopRight;
	glm::ivec2 newMapBottomLeft;
	glm::ivec2 newMapTopRight;

	glm::ivec2 oldPlayableBottomLeft;
	glm::ivec2 oldPlayableTopRight;
	glm::ivec2 newPlayableBottomLeft;
	glm::ivec2 newPlayableTopRight;

	Texture originalMinimap;
};
