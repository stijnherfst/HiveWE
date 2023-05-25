#pragma once

#include "ui_map_info_editor.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class MapInfoEditor : public QDialog {
	Q_OBJECT

public:
	MapInfoEditor(QWidget* parent = nullptr);

	Ui::MapInfoEditor ui;

	void save() const;
};