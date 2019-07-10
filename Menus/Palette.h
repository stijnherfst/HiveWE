#pragma once

#include <QDialog>

#include "QRibbon.h"

/// Palette is the base for all other palette kinds and facilitates things like brush switching and shortcut management
class Palette : public QDialog {
	Q_OBJECT

public:
	Palette(QWidget* parent = nullptr);
	~Palette();

signals:
	void ribbon_tab_requested(QRibbonTab* tab, QString name);

	virtual void deactivate(QRibbonTab* tab) = 0;
};
