#pragma once

#include <QDialog>

import QRibbon;

/// Palette is the base for all other palette kinds and facilitates things like brush switching and shortcut management
class Palette : public QDialog {
	Q_OBJECT

public:
	Palette(QWidget* parent = nullptr);
	~Palette();

	std::vector<QShortcut*> shortcuts;

	//void addShortcut(const QKeySequence sequence, const std::vector<QWidget*>& attach_to);

signals:
	void ribbon_tab_requested(QRibbonTab* tab, QString name);

public slots:
	virtual void deactivate(QRibbonTab* tab) = 0;
};
