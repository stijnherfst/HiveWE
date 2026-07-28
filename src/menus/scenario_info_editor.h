#pragma once

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
	void restoreDefaults();
	void restorePlayerProperties();
    
	// Private redefinition of Controller type for mapping dropdown indices
	// PlayerType (equivalent enum) lacks the none type
	enum ControllerType {
		none,
		user,
		computer,
		neutral,
		rescuable
	};
};
