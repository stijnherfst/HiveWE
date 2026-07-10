#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>

#include "ui_scenario_info_editor.h"

struct PlayerRow {
	QLineEdit* name;
	QLabel* color;
	QComboBox* race;
	QComboBox* controller;
	QCheckBox* fixed_start_position;
};

class ScenarioInfoEditor: public QDialog {
	Q_OBJECT

  public:
	ScenarioInfoEditor(QWidget* parent = nullptr);

	Ui::ScenarioInfoEditor ui;
	
	std::vector<PlayerRow> player_rows;

	bool save() const;

  private:
	void updateController(int slotIndex, int controllerTypeIndex);
	void updateRace(int slotIndex, int raceTypeIndex);
    
};
