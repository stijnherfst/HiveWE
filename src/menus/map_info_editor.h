#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ui_map_info_editor.h"

import Texture;

class MapInfoEditor : public QDialog {
	Q_OBJECT

public:
	MapInfoEditor(QWidget* parent = nullptr);

	Ui::MapInfoEditor ui;

	void save() const;

private:
	void updateSizeDisplays();
	void adjustBounds(int deltaLeft, int deltaRight, int deltaTop, int deltaBottom);

	// used for changing map size
	glm::vec2 oldMapBottomLeft;
	glm::vec2 oldMapTopRight;
	glm::vec2 newMapBottomLeft;
	glm::vec2 newMapTopRight;

	glm::vec2 oldPlayableBottomLeft;
	glm::vec2 oldPlayableTopRight;
	glm::vec2 newPlayableBottomLeft;
	glm::vec2 newPlayableTopRight;

	Texture newMinimapTex;
};