#pragma once

#include <QDialog>
#include <QLineEdit>

import GameplayConstants;
import TriggerStrings;

class GameplayConstantsEditor : public QDialog {
	Q_OBJECT
public:
	GameplayConstantsEditor(QWidget* parent, GameplayConstants& constants, TriggerStrings& trigger_strings);

private:
	QLineEdit* search;
};
