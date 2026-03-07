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
	void updateMapSizeGUI();
	void adjustBounds(int deltaLeft, int deltaRight, int deltaTop, int deltaBottom);
	void updateBoundsPreview();
	void updateBoundsText();

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
